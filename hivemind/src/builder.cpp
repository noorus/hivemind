#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "builder.h"
#include "database.h"

namespace hivemind {

  Builder::Builder( Bot* bot ): Subsystem( bot ), idPool_( 0 )
  {
  }

  void Builder::gameBegin()
  {
    buildings_.clear();
    idPool_ = 0;
    bot_->messaging().listen( Listen_Global, this );
  }

  bool Builder::add( UnitTypeID structure, const Base& base, AbilityID ability, BuildProjectID& idOut )
  {
    Vector2 pos;
    if ( !findPlacement( structure, base, BuildPlacement_Generic, ability, pos ) )
      return false;

    Building build( idPool_++, structure, ability );
    build.position = pos;
    bot_->map().reserveFootprint( build.position, structure );
    buildings_.push_back( build );
    bot_->console().printf( "Builder: New BuildOp %d for %s, position %d,%d", build.id, sc2::UnitTypeToName( structure ), build.position.x, build.position.y );

    idOut = build.id;
    return true;
  }

  void Builder::remove( BuildProjectID id )
  {
    for ( auto& building : buildings_ )
      if ( building.id == id )
        building.cancel = true;
  }

  void Builder::gameEnd()
  {
    bot_->messaging().remove( this );
  }

  void Builder::draw()
  {
    for ( auto& b : buildings_ )
    {
      Vector3 pt( (Real)b.position.x + 0.5f, (Real)b.position.y + 0.5f, bot_->map().maxZ_ );
      bot_->debug().drawSphere( pt, 1.5f, sc2::Colors::Red );
      bot_->debug().drawText( std::to_string( b.id ), pt, sc2::Colors::Yellow );
      if ( b.builder && ( !b.building || !b.building->is_alive ) )
      {
        bot_->debug().drawSphere( b.builder->pos, 0.85f, sc2::Colors::Red );
        bot_->debug().drawText( std::to_string( b.id ), Vector3( b.builder->pos ), sc2::Colors::Yellow );
      }
    }
  }

  const Real cBuildDistHeur = 1.0f;

  void Builder::onMessage( const Message& msg )
  {
    if ( msg.code == M_Global_UnitCreated )
    {
      if ( !utils::isMine( msg.unit() ) )
        return;
      if ( !Database::unit( msg.unit()->unit_type ).structure )
        return;
      for ( auto& build : buildings_ )
      {
        if ( !build.building && build.type == msg.unit()->unit_type )
        {
          Vector2 pos = msg.unit()->pos;
          auto dist = pos.distance( build.position );
          if ( dist < cBuildDistHeur )
          {
            bot_->console().printf( "BuildOp %d: Got building %x at pos %f,%f", build.id, msg.unit(), pos.x, pos.y );
            build.building = msg.unit();
            build.buildStartTime = bot_->time();
            bot_->messaging().sendGlobal( M_Build_Started, build.id );
            return;
          }
        }
      }
    }
    else if ( msg.code == M_Global_ConstructionCompleted )
    {
      if ( !utils::isMine( msg.unit() ) )
        return;
      if ( !Database::unit( msg.unit()->unit_type ).structure )
        return;
      for ( auto& build : buildings_ )
      {
        if ( build.building == msg.unit() )
        {
          build.completed = true;
          build.buildCompleteTime = bot_->time();
          bot_->console().printf( "BuildOp %d: Completed in time %d", build.id, build.buildCompleteTime - build.buildStartTime );
        }
      }
    }
    else if ( msg.code == M_Global_UnitDestroyed )
    {
      if ( !utils::isMine( msg.unit() ) || !utils::isWorker( msg.unit() ) )
        return;
      for ( auto& build : buildings_ )
      {
        if ( build.builder == msg.unit() )
          build.builder = nullptr;
      }
    }
  }

  const GameTime cBuildRecheckDelta = 50;
  const GameTime cBuildReorderDelta = 100;
  const size_t cBuildMaxWorkers = 3;
  const size_t cBuildMaxOrders = 3;

  void Builder::update( const GameTime time, const GameTime delta )
  {
    auto it = buildings_.begin();
    while ( it != buildings_.end() )
    {
      auto& build = ( *it );
      if ( build.completed || build.cancel )
      {
        if ( build.completed )
          bot_->messaging().sendGlobal( M_Build_Finished, build.id );
        else
          bot_->messaging().sendGlobal( M_Build_Canceled, build.id );

        bot_->console().printf( "Builder: Removing BuildOp %d (%s)", build.id, build.completed ? "completed" : "canceled" );

        // there's a bug in the current API where drones which became buildings are still marked as alive.
        if ( build.builder && build.builder->is_alive && build.cancel )
        {
          bot_->console().printf( "Builder: Returning worker %x", build.builder );
          bot_->workers().addBack( build.builder );
        }
        bot_->map().clearFootprint( build.position );
        it = buildings_.erase( it );
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
            bot_->console().printf( "BuildOp %d: Canceling after %d failed tries (no worker)", build.id, build.tries );
            build.cancel = true;
            continue;
          }
          build.builder = bot_->workers().release();
          if ( !build.builder )
          {
            bot_->console().printf( "BuildOp %d: No worker released from pool", build.id );
            continue;
          }
          bot_->unitDebugMsgs_[build.builder] = "Builder, Op " + std::to_string( build.id );
          bot_->console().printf( "BuildOp %d: Got worker %x", build.id, build.builder );
          bot_->action().UnitCommand( build.builder, sc2::ABILITY_ID::MOVE, build.position, false );
          build.tries++;
        }

        if ( !build.building || !build.building->is_alive )
        {
          bool hasOrder = false;
          for(auto& order : build.builder->orders)
          {
            if(order.ability_id == build.buildAbility)
            {
              hasOrder = true;
              break;
            }
          }

          if ( !hasOrder || ( build.lastOrderTime + cBuildReorderDelta < time ) )
          {
            if ( build.orderTries >= cBuildMaxOrders )
            {
              bot_->console().printf( "BuildOp %d: Canceling after %d failed tries (order failed)", build.id, build.orderTries );
              build.cancel = true;
              continue;
            }
            bot_->action().UnitCommand( build.builder, build.buildAbility, build.position, false );
            build.lastOrderTime = time;
            build.orderTries++;
          }
        }
      }
    }
  }

  bool Builder::findPlacement( UnitTypeID structure, const Base& base, BuildingPlacement type, AbilityID ability, Vector2& placementOut )
  {
    assert( type == BuildPlacement_Generic );

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
          Vector2 tileRet = tile;
          if ( !bot_->query().Placement( ability, tileRet ) )
            continue;
          placementOut = tile;
          return true;
        }
      }
    }

    return false;
  }

  std::pair<int,int> Builder::getAllocatedResources() const
  {
    int mineralSum = 0;
    int vespeneSum = 0;
    for(auto& building : buildings_)
    {
      if(building.moneyAllocated)
      {
        continue;
      }

      const auto& data = Database::units().at(building.type);
      mineralSum += data.mineralCost;
      vespeneSum += data.vespeneCost;
    }
    return { mineralSum, vespeneSum };
  }
}