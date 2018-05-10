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
    trainer_( bot, idPool_, unitStats_ ),
    researcher_( bot, idPool_, upgradeStats_ )
  {
  }

  void Builder::gameBegin()
  {
    buildProjects_.clear();
    idPool_ = 0;
    bot_->messaging().listen( Listen_Global, this );

    trainer_.gameBegin();
    researcher_.gameBegin();
  }

  bool Builder::make(UnitTypeID unitType, Base* base, BuildProjectID& idOut)
  {
    auto trainerType = getTrainerType(unitType);

    if(!utils::isStructure(unitType) || utils::isStructure(trainerType))
    {
      return train(unitType, base, idOut);
    }
    else
    {
      BuildingPlacement placement = BuildPlacement_Generic;

      if(utils::isMainStructure(unitType))
        placement = BuildPlacement_MainBuilding;

      if(utils::isRefinery(unitType))
        placement = BuildPlacement_Extractor;

      return build(unitType, base, placement, idOut);
    }
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

  bool Builder::train(UnitTypeID unitType, Base* base, BuildProjectID& idOut)
  {
    return trainer_.train(unitType, base, idOut);
  }

  bool Builder::research(UpgradeID upgradeType, Base* base, UnitTypeID researcherType, BuildProjectID& idOut)
  {
    return researcher_.research(upgradeType, base, researcherType, idOut);
  }

  void Builder::remove( BuildProjectID id )
  {
    for ( auto& building : buildProjects_ )
      if ( building.id == id )
        building.cancel = true;

    trainer_.remove(id);
    researcher_.remove(id);
  }

  void Builder::gameEnd()
  {
    trainer_.gameEnd();
    researcher_.gameEnd();

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

    Point2D screenPosition = Point2D(0.03f, 0.5f);
    const Point2D increment( 0.0f, 0.011f );
    for(auto& stats : unitStats_)
    {
      if(stats.first == sc2::UNIT_TYPEID::ZERG_LARVA) // Larva counts are not properly tracked.
        continue;
      if(stats.first == sc2::UNIT_TYPEID::ZERG_EGG) // Egg counts are always 0.
        continue;

      char text[128];
      if(stats.second.inProgressCount() > 0)
        sprintf_s( text, 128, "%s: %d (+%d)", sc2::UnitTypeToName(stats.first), stats.second.unitCount(), stats.second.inProgressCount() );
      else
        sprintf_s( text, 128, "%s: %d", sc2::UnitTypeToName(stats.first), stats.second.unitCount() );
      debug.drawText( text, screenPosition, sc2::Colors::Yellow );
      screenPosition += increment;
    }
    screenPosition += increment;

    for(auto& stats : upgradeStats_)
    {
      char text[128];
      const char* progress = stats.second == UpgradeStatus::NotStarted ? "0" : stats.second == UpgradeStatus::inProgress ? "0 (+1)" : "1";
      sprintf_s( text, 128, "%s: %s", sc2::UpgradeIDToName(stats.first), progress );
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
            build.building = unit;
            build.buildStartTime = bot_->time();

            if(g_CVar_builder_debug.as_i() > 1)
            {
              auto time = bot_->timeSeconds();
              auto seconds = time % 60;
              auto minutes = time / 60;

              bot_->console().printf("BuildOp %d for %s: Started building %x at pos (%d,%d) at game time %02d:%02d", build.id, sc2::UnitTypeToName(build.type), id(unit), (int)pos.x, (int)pos.y, minutes, seconds);
            }

            if(build.builder->unit_type == sc2::UNIT_TYPEID::ZERG_DRONE)
            {
              auto& droneStats = unitStats_[sc2::UNIT_TYPEID::ZERG_DRONE];
              droneStats.units.erase(build.builder);
            }

            bot_->messaging().sendGlobal( M_Build_Started, build.id );
            return;
          }
        }
      }
    }
    else if ( msg.code == M_Global_ConstructionCompleted )
    {
      UnitRef unit = msg.unit();

      if ( !utils::isMine( unit ) )
        return;
      if ( !utils::isStructure( unit ) )
        return;

      auto& stats = unitStats_[unit->unit_type];
      stats.units.insert(unit);

      for ( auto& build : buildProjects_ )
      {
        if ( build.building == unit )
        {
          build.completed = true;
          build.buildCompleteTime = bot_->time();

          if ( g_CVar_builder_debug.as_i() > 1 )
            bot_->console().printf( "BuildOp %d for %s: Completed in time %d", build.id, sc2::UnitTypeToName( build.type ), build.buildCompleteTime - build.buildStartTime );
        }
      }
    }
    else if(msg.code == M_Global_UnitDestroyed)
    {
      UnitRef unit = msg.unit();

      if(!utils::isMine(unit))
        return;

      if(utils::isWorker(unit))
      {
        for(auto& build : buildProjects_)
        {
          if(build.builder == unit)
            build.builder = nullptr;
        }
      }
      else if(utils::isStructure(unit))
      {
        for ( auto& build : buildProjects_ )
        {
          if ( build.building == unit )
          {
            build.cancel = true;
            build.buildCompleteTime = bot_->time();

            if ( g_CVar_builder_debug.as_i() > 1 )
              bot_->console().printf( "BuildOp %d for %s: canceled (or destroyed) in time %d", build.id, sc2::UnitTypeToName( build.type ), build.buildCompleteTime - build.buildStartTime );

            if(build.builder && build.builder->is_alive && build.builder->unit_type == sc2::UNIT_TYPEID::ZERG_DRONE)
            {
              auto& droneStats = unitStats_[sc2::UNIT_TYPEID::ZERG_DRONE];
              droneStats.units.insert(build.builder);
            }
          }
        }
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
        auto& stats = unitStats_[build.type];
        stats.inProgress.erase(build.id);

        if ( build.completed )
          bot_->messaging().sendGlobal( M_Build_Finished, build.id );
        else
          bot_->messaging().sendGlobal( M_Build_Canceled, build.id );

        if ( verbose )
          bot_->console().printf( "BuildOp %d for %s (%s):", build.id, sc2::UnitTypeToName( build.type ), build.completed ? "completed" : "canceled" );

        if ( build.builder && build.builder->is_alive )
        {
          if ( verbose )
            bot_->console().printf( "Builder: Returning worker %x", id( build.builder ) );

          build.base->addWorker(build.builder);
        }
        bot_->map().clearFootprint( build.position );
        finishedBuildProjects_[build.id] = int(build.completed);

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
          if ( build.workerTries >= cBuildMaxWorkers )
          {
            if ( verbose )
              bot_->console().printf( "BuildOp %d for %s: Canceling after %d failed tries (no worker)", build.id, sc2::UnitTypeToName( build.type ), build.workerTries );

            build.cancel = true;
            continue;
          }

          ++build.workerTries;

          build.builder = acquireBuilder(*build.base);

          if ( !build.builder )
          {
            if ( verbose )
              bot_->console().printf( "BuildOp %d for %s: Failed to acquire worker", build.id, sc2::UnitTypeToName( build.type ) );

            continue;
          }
          bot_->unitDebugMsgs_[build.builder] = "Builder, Op " + std::to_string( build.id );

          if ( verbose )
            bot_->console().printf( "BuildOp %d for %s: Got worker %x", build.id, sc2::UnitTypeToName( build.type ), id( build.builder ) );

          Drone( build.builder ).move( build.position );
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
                bot_->console().printf( "BuildOp %d for %s: Canceling after %d failed tries (order failed)", build.id, sc2::UnitTypeToName( build.type ), build.orderTries );

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
    researcher_.update(time, delta);
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
        if(!bot_->map().canZergBuild(structure, tile, 1, true, true, true, true))
          continue;

        const Vector2& tileRet = tile;
        if ( !bot_->query().Placement( ability, tileRet ) )
          continue;

        placementOut = tile;
        return true;
      }
    }
    else if ( type == BuildPlacement_Extractor )
    {
      auto position = base.location()->position_;

      auto observedGeysers = bot_->observation().GetUnits( Unit::Alliance::Neutral, []( auto unit ) { return utils::isGeyser( unit ); } );

      for ( auto& geyser : base.location()->getGeysers() )
      {
        Vector2 tile = geyser->pos;
        if(!bot_->map().canZergBuild(structure, tile, 0, true, true, true, true))
          continue;

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

  UnitTypeID getTrainerType(UnitTypeID unitType)
  {
    if(unitType == sc2::UNIT_TYPEID::ZERG_LAIR)
    {
      return sc2::UNIT_TYPEID::ZERG_HATCHERY;
    }
    else if(unitType == sc2::UNIT_TYPEID::ZERG_HIVE)
    {
      return sc2::UNIT_TYPEID::ZERG_LAIR;
    }
    else if(unitType == sc2::UNIT_TYPEID::ZERG_GREATERSPIRE)
    {
      return sc2::UNIT_TYPEID::ZERG_SPIRE;
    }
    else if(utils::isStructure(unitType))
    {
      return sc2::UNIT_TYPEID::ZERG_DRONE;
    }
    else if(unitType == sc2::UNIT_TYPEID::ZERG_QUEEN)
    {
      return sc2::UNIT_TYPEID::ZERG_HATCHERY;
    }
    else
    {
      return sc2::UNIT_TYPEID::ZERG_LARVA;
    }
  }

  AllocatedResources Builder::getCost(UnitTypeID unitType)
  {
    UnitTypeID trainerType = getTrainerType(unitType);

    auto& ability = Database::techTree().getBuildAbility(unitType, trainerType);

    // Zerg buildings cost negative supply because they consume the drone.
    int supplyCost = std::max(ability.supplyCost, 0);

    return { ability.mineralCost, ability.vespeneCost, supplyCost };
  }

  AllocatedResources Builder::getAllocatedResources() const
  {
    AllocatedResources builderResources = { 0, 0, 0 };
    for(auto& building : buildProjects_)
    {
      if(building.moneyAllocated)
      {
        continue;
      }

      auto cost = getCost(building.type);

      builderResources.minerals += cost.minerals;
      builderResources.vespene += cost.vespene;
      builderResources.food += cost.food;
    }

    auto trainerResources = trainer_.getAllocatedResources();
    auto researcherResources = researcher_.getAllocatedResources();

    auto m = builderResources.minerals + trainerResources.minerals + researcherResources.minerals;
    auto v = builderResources.vespene + trainerResources.vespene + researcherResources.vespene;
    auto f = builderResources.food + trainerResources.food + researcherResources.food;

    return { m, v, f };
  }

  bool Builder::haveResourcesToMake(UnitTypeID unitType, AllocatedResources allocatedResources) const
  {
    int minerals = bot_->Observation()->GetMinerals();
    int vespene = bot_->Observation()->GetVespene();
    int supplyLimit = bot_->Observation()->GetFoodCap();
    int usedSupply = bot_->Observation()->GetFoodUsed();

    auto& baseManager = bot_->bases();
    auto& builder = bot_->builder();

    auto builderAllocatedResources = builder.getAllocatedResources();
    minerals -= builderAllocatedResources.minerals;
    vespene -= builderAllocatedResources.vespene;
    usedSupply += builderAllocatedResources.food;

    minerals -= allocatedResources.minerals;
    vespene -= allocatedResources.vespene;
    usedSupply += allocatedResources.food;

    auto cost = getCost(unitType);

    bool haveMoney = minerals >= cost.minerals;
    bool haveGas = vespene >= cost.vespene;
    bool haveSupply = usedSupply + cost.food <= supplyLimit;

    //bot_->console().printf("Cost of %s is %d minerals, %d vespene, %d supply", sc2::UnitTypeToName( unitType ), cost.minerals, cost.vespene, cost.food);

    return haveMoney && haveGas && haveSupply;
  }

}
