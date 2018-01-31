#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "builder.h"
#include "database.h"
#include "controllers.h"
#include "console.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( builder_debug, "Whether to show and print debug information on the builder subsystem. 0 = none, 1 = basics, 2 = verbose", 1 );

  Builder::Builder( Bot* bot ):
    Subsystem( bot ),
    idPool_( 0 ),
    trainer_( bot, idPool_, unitStats_ )
  {
  }

  void Builder::gameBegin()
  {
    buildProjects_.clear();
    idPool_ = 0;
    bot_->messaging().listen( Listen_Global, this );

    trainer_.gameBegin();
  }

  bool Builder::build( UnitTypeID structureType, Base* base, BuildingPlacement placement, BuildProjectID& idOut )
  {
    auto ability = Database::techTree().getBuildAbility( structureType, sc2::UNIT_TYPEID::ZERG_DRONE ).ability;

    Vector2 pos;
    UnitRef target = nullptr;
    if ( !findPlacement( structureType, *base, placement, ability, pos, target ) )
      return false;

    Building build( idPool_++, base, structureType, ability );
    build.position = pos;
    build.target = target;
    bot_->map().reserveFootprint( build.position, structureType );
    buildProjects_.push_back( build );

    if ( g_CVar_builder_debug.as_i() > 1 )
      bot_->console().printf( "Builder: New BuildOp %d for %s, position %d,%d", build.id, sc2::UnitTypeToName( structureType ), build.position.x, build.position.y );

    unitStats_[structureType].inProgress.insert(build.id);

    idOut = build.id;
    return true;
  }

  bool Builder::train(UnitTypeID unitType, Base* base, UnitTypeID trainerType, BuildProjectID& idOut)
  {
    return trainer_.train(unitType, base, trainerType, idOut);
  }

  void Builder::remove( BuildProjectID id )
  {
    for ( auto& building : buildProjects_ )
      if ( building.id == id )
        building.cancel = true;

    trainer_.remove(id);
  }

  void Builder::gameEnd()
  {
    trainer_.gameEnd();

    bot_->messaging().remove( this );
  }

  void Builder::draw()
  {
    if ( g_CVar_builder_debug.as_i() < 1 )
      return;

    auto& debug = bot_->debug();

    for ( auto& b : buildProjects_ )
    {
      Real height = bot_->map().heightMap_[b.position.x][b.position.y];
      Vector3 pt( (Real)b.position.x + 0.5f, (Real)b.position.y + 0.5f, height );
      debug.drawSphere( pt, 1.5f, sc2::Colors::Red );
      debug.drawText( std::to_string( b.id ), pt, sc2::Colors::Yellow );
      if ( b.builder && ( !b.building || !b.building->is_alive ) )
      {
        debug.drawSphere( b.builder->pos, 0.85f, sc2::Colors::Red );
        debug.drawText( std::to_string( b.id ), Vector3( b.builder->pos ), sc2::Colors::Yellow );
      }
    }

    trainer_.draw();

    Point2D screenPosition = Point2D(0.03f, 0.5f);
    const Point2D increment( 0.0f, 0.011f );
    for(auto& stats : unitStats_)
    {
      if(stats.first == sc2::UNIT_TYPEID::ZERG_LARVA) // Larva counts are not properly tracked.
        continue;
      if(stats.first == sc2::UNIT_TYPEID::ZERG_EGG) // Egg counts are always 0.
        continue;

      char text[128];
      sprintf_s( text, 128, "%s: %d (+%d)", sc2::UnitTypeToName(stats.first), stats.second.unitCount(), stats.second.inProgressCount() );
      debug.drawText( text, screenPosition, sc2::Colors::Yellow );
      screenPosition += increment;
    }
  }

  const Real cBuildDistHeur = 1.0f;

  void Builder::onMessage( const Message& msg )
  {
    if ( msg.code == M_Global_UnitCreated )
    {
      UnitRef unit = msg.unit();

      if ( !utils::isMine( unit ) )
        return;
      if ( !utils::isStructure(unit) )
        return;

      auto& stats = unitStats_[unit->unit_type];

      if(unit->build_progress == 1.0f) // The initial hatchery is created without construction.
      {
        stats.units.insert(unit);
      }

      for ( auto& build : buildProjects_ )
      {
        if ( !build.building && build.type == unit->unit_type )
        {
          Vector2 pos = unit->pos;
          auto dist = pos.distance( build.position );
          if ( dist < cBuildDistHeur )
          {
            if ( g_CVar_builder_debug.as_i() > 1 )
              bot_->console().printf( "BuildOp %d: Got building %x at pos %f,%f", build.id, id( unit ), pos.x, pos.y );

            build.building = unit;
            build.buildStartTime = bot_->time();
            bot_->messaging().sendGlobal( M_Build_Started, build.id );
            return;
          }
        }
      }
    }
    else if ( msg.code == M_Global_ConstructionCompleted )
    {
      UnitRef unit = msg.unit();

      if ( !utils::isMine( msg.unit() ) )
        return;
      if ( !Database::unit( msg.unit()->unit_type ).structure )
        return;

      auto& stats = unitStats_[unit->unit_type];

      for ( auto& build : buildProjects_ )
      {
        if ( build.building == msg.unit() )
        {
          build.completed = true;
          build.buildCompleteTime = bot_->time();

          if ( g_CVar_builder_debug.as_i() > 1 )
            bot_->console().printf( "BuildOp %d: Completed in time %d", build.id, build.buildCompleteTime - build.buildStartTime );

          stats.units.insert(unit);
          stats.inProgress.erase(build.id);
        }
      }
    }
    else if ( msg.code == M_Global_UnitDestroyed )
    {
      if ( !utils::isMine( msg.unit() ) || !utils::isWorker( msg.unit() ) )
        return;
      for ( auto& build : buildProjects_ )
      {
        if ( build.builder == msg.unit() )
          build.builder = nullptr;
      }
    }
  }

  UnitRef Builder::acquireBuilder(Base& base)
  {
    return base.releaseWorker();
  }

  const GameTime cBuildRecheckDelta = 50;
  const GameTime cBuildReorderDelta = 100;
  const size_t cBuildMaxWorkers = 3;
  const size_t cBuildMaxOrders = 3;

  void Builder::update( const GameTime time, const GameTime delta )
  {
    auto verbose = ( g_CVar_builder_debug.as_i() > 1 );

    auto it = buildProjects_.begin();
    while ( it != buildProjects_.end() )
    {
      auto& build = ( *it );
      if ( build.completed || build.cancel )
      {
        if ( build.completed )
          bot_->messaging().sendGlobal( M_Build_Finished, build.id );
        else
          bot_->messaging().sendGlobal( M_Build_Canceled, build.id );

        if ( verbose )
          bot_->console().printf( "Builder: Removing BuildOp %d (%s)", build.id, build.completed ? "completed" : "canceled" );

        if ( build.builder && build.builder->is_alive )
        {
          if ( verbose )
            bot_->console().printf( "Builder: Returning worker %x", id( build.builder ) );

          build.base->addWorker(build.builder);
        }
        bot_->map().clearFootprint( build.position );
        it = buildProjects_.erase( it );
        continue;
      }
      it++;

      bool workerHasBuildOrder = false;
      bool buildingStarted = build.building && build.building->is_alive;
      if(!buildingStarted && build.builder && build.builder->is_alive)
      {
        for(auto& order : build.builder->orders)
        {
          if(order.ability_id == build.buildAbility)
          {
            workerHasBuildOrder = true;
            break;
          }
        }
      }
      build.moneyAllocated = buildingStarted || workerHasBuildOrder;

      if ( build.nextUpdateTime <= time )
      {
        build.nextUpdateTime = time + cBuildRecheckDelta;
        if ( !build.builder || !build.builder->is_alive )
        {
          if ( build.tries >= cBuildMaxWorkers )
          {
            if ( verbose )
              bot_->console().printf( "BuildOp %d: Canceling after %d failed tries (no worker)", build.id, build.tries );

            build.cancel = true;
            continue;
          }

          build.builder = acquireBuilder(*build.base);

          if ( !build.builder )
          {
            if ( verbose )
              bot_->console().printf( "BuildOp %d: No worker released from pool", build.id );

            continue;
          }
          bot_->unitDebugMsgs_[build.builder] = "Builder, Op " + std::to_string( build.id );

          if ( verbose )
            bot_->console().printf( "BuildOp %d: Got worker %x", build.id, id( build.builder ) );

          Drone( build.builder ).move( build.position );
          build.tries++;
        }

        if ( !build.building || !build.building->is_alive )
        {
          bool hasOrder = false;
          for ( auto& order : build.builder->orders )
          {
            if ( order.ability_id == build.buildAbility )
            {
              hasOrder = true;
              break;
            }
          }

          if ( !hasOrder && ( build.lastOrderTime + cBuildReorderDelta < time ) )
          {
            if ( build.orderTries >= cBuildMaxOrders )
            {
              if ( verbose )
                bot_->console().printf( "BuildOp %d: Canceling after %d failed tries (order failed)", build.id, build.orderTries );

              build.cancel = true;
              continue;
            }
            if ( build.target )
            {
              Drone( build.builder ).build( build.buildAbility, build.target );
            }
            else
            {
              Drone( build.builder ).build( build.buildAbility, build.position );
            }
            build.lastOrderTime = time;
            build.orderTries++;
          }
        }
      }
    }

    trainer_.update(time, delta);
  }

  bool Builder::findPlacement( UnitTypeID structure, const Base& base, BuildingPlacement type, AbilityID ability, Vector2& placementOut, UnitRef& targetOut )
  {
    assert( type == BuildPlacement_Generic || type == BuildPlacement_Extractor || type == BuildPlacement_MainBuilding);

    if ( type == BuildPlacement_Generic )
    {
      UnitRef depot = nullptr;
      if ( !base.depots().empty() )
        depot = *base.depots().cbegin();

      Vector2 pos = ( depot ? depot->pos : base.location()->position() );

      auto& closestTiles = bot_->map().getDistanceMap( pos ).sortedTiles();

      for ( auto& tile : closestTiles )
      {
        if ( bot_->map().canZergBuild( structure, tile, 1, true, true, true, true ) )
        {
          const Vector2& tileRet = tile;
          if ( !bot_->query().Placement( ability, tileRet ) )
            continue;
          placementOut = tile;
          return true;
        }
      }
    }
    else if ( type == BuildPlacement_Extractor )
    {
      auto position = base.location()->position_;

      auto observedGeysers = bot_->observation().GetUnits( Unit::Alliance::Neutral, []( auto unit ) { return utils::isGeyser( unit ); } );

      for ( auto& geyser : base.location()->getGeysers() )
      {
        if ( !bot_->query().Placement( ability, geyser->pos ) )
          continue;

        placementOut = geyser->pos;
        targetOut = geyser;

        // Don't return snapshot geysers.
        for(auto observedGeyser : observedGeysers)
        {
          if(Vector3(observedGeyser->pos).distance(geyser->pos) < 1.0f)
          {
            targetOut = observedGeyser;
            break;
          }
        }

        return true;
      }
    }
    else if(type == BuildPlacement_MainBuilding)
    {
      auto oldBasePosition = base.location()->position_;

      int bestDistance = std::numeric_limits<int>::max();
      auto bestPosition = oldBasePosition;

      auto& baseLocations = bot_->map().getBaseLocations();
      for(auto& baseLocation : baseLocations)
      {
        auto newBasePosition = baseLocation.position_;

        if(!bot_->query().Placement(ability, newBasePosition))
          continue;

        int distance = base.location()->distanceMap_.dist(newBasePosition);

        if(distance <= bestDistance)
        {
          bestDistance = distance;
          bestPosition = newBasePosition;
        }
      }

      if(bestDistance != std::numeric_limits<int>::max())
      {
        placementOut = bestPosition;
        return true;
      }
    }

    return false;
  }

  std::pair<int,int> Builder::getAllocatedResources() const
  {
    int mineralSum = 0;
    int vespeneSum = 0;
    for(auto& building : buildProjects_)
    {
      if(building.moneyAllocated)
      {
        continue;
      }

      const auto& data = Database::units().at(building.type);
      mineralSum += data.mineralCost;
      vespeneSum += data.vespeneCost;
    }

    auto s = trainer_.getAllocatedResources();
    return { s.first + mineralSum, s.second + vespeneSum };
  }
}
