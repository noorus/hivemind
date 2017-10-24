#include "stdafx.h"
#include "basemanager.h"
#include "bot.h"
#include "exception.h"
#include "utilities.h"

namespace hivemind {

  void BaseManager::debugSpawnQueen( UnitRef depot )
  {
    bot_->console().printf( "...and here's your complimentary queen" );
    Vector2 qpos = depot->pos;
    qpos.y -= depot->radius + 0.75f;
    bot_->debug().DebugCreateUnit( UNIT_TYPEID::ZERG_QUEEN, qpos, bot_->players().self().id(), 1 );
  }

  BaseManager::BaseManager( Bot* bot ): Subsystem( bot )
  {
  }

  static bool firstFrame = true;

  bool BaseManager::addBase( UnitRef depot )
  {
    BaseLocation* location = nullptr;
    for ( auto& it : bot_->map().baseLocations_ )
      if ( it.containsPosition( depot->pos ) )
      {
        location = &it;
        break;
      }

    if ( !location )
      return false;

    bot_->console().printf( "Adding base (%s) at location %llu", sc2::UnitTypeToName( depot->unit_type ), location->baseID_ );

    bases_.emplace_back( this, bases_.size(), location, depot );

    if ( !firstFrame )
      debugSpawnQueen( depot );

    return true;
  }

  void BaseManager::gameBegin()
  {
    firstFrame = true;

    // add initial base here because we can't know the order of initial creation events for adding units to the base.
    for ( auto unit : bot_->observation().GetUnits( sc2::Unit::Alliance::Self ) )
    {
      if ( utils::isMainStructure( unit ) )
        addBase( unit );
    }

    bot_->messaging().listen( Listen_Global, this );
  }

  void BaseManager::gameEnd()
  {
    bases_.clear();
    assignedUnits_.clear();
  }

  void BaseManager::update( const GameTime time, const GameTime delta )
  {
    for ( auto& base : bases_ )
      base.update( *bot_ );

    if ( firstFrame )
    {
      debugSpawnQueen( *bases_[0].depots().cbegin() );
      firstFrame = false;
    }
  }

  void BaseManager::draw()
  {
    for ( auto& base : bases_ )
      base.draw( bot_ );
  }

  void BaseManager::onMessage( const Message& msg )
  {
    if ( msg.code == M_Global_UnitCreated && utils::isMine( msg.unit() ) )
    {
      // Hack to avoid double events in the beginning.
      if ( assignedUnits_.find( msg.unit() ) != assignedUnits_.end() )
        return;
      assignedUnits_.insert( msg.unit() );

      if ( utils::isBuilding( msg.unit() ) )
      {
        bot_->console().printf( "Building created: %s", sc2::UnitTypeToName( msg.unit()->unit_type ) );
        addBuilding( msg.unit() );
      }
      else
      {
        // Workers are added through separate AddWorker message from WorkerManager
        auto base = findClosest( msg.unit()->pos );
        if ( msg.unit()->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_LARVA && base )
        {
          bot_->console().printf( "Base %llu: adding larva %I64x", base->id(), msg.unit() );
          base->addLarva( msg.unit() );
        }
        else if ( msg.unit()->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_QUEEN && base )
        {
          bot_->console().printf( "Base %llu: adding queen %I64x", base->id(), msg.unit() );
          base->addQueen( msg.unit() );
        }
      }
    }
    else if ( msg.code == M_Global_AddWorker )
    {
      addWorker( msg.unit() );
    }
    else if ( msg.code == M_Global_RemoveWorker )
    {
      removeWorker( msg.unit() );
    }
    else if ( msg.code == M_Global_UnitDestroyed && utils::isMine( msg.unit() ) )
    {
      assignedUnits_.erase( msg.unit() );
      if ( !utils::isWorker( msg.unit() ) )
        for ( auto& base : bases_ )
          base.remove( msg.unit() );
    }
  }

  Base* BaseManager::findClosest( const Vector3 & pos )
  {
    Base* ret = nullptr;
    Real minDist = 32000.0f;
    for ( auto& base : bases_ )
    {
      auto dist = base.location()->position().squaredDistance( pos.to2() );
      if ( dist < minDist )
      {
        minDist = dist;
        ret = &base;
      }
    }

    return ret;
  }

  void BaseManager::addBuilding( UnitRef building )
  {
    Base* base = nullptr;
    if ( utils::isMainStructure( building ) )
    {
      // if new main structure is inside an existing base's area, add it to that base.
      // otherwise if building is inside a base location's area, create a new base with that location.
      // otherwise just add it to whatever is the closest base.
      // TODO Find base by polygonal region rather than distance, distance throws off way too easily.
      depots_.insert( building );
      for ( auto& it : bases_ )
        if ( it.location()->containsPosition( building->pos ) )
          base = &it;
      if ( base )
        base->addDepot( building );
      else
      {
        if ( !addBase( building ) )
        {
          base = findClosest( building->pos );
          if ( base )
            base->addDepot( building );
        }
      }
    }
    else
    {
      base = findClosest( building->pos );
      if ( base )
        base->addBuilding( building );
    }
  }

  void BaseManager::addWorker( UnitRef unit )
  {
    // TODO Find base by need vs. distance
    auto base = findClosest( unit->pos );
    if ( base )
    {
      bot_->console().printf( "Base %llu: adding worker %I64x", base->id(), unit );
      base->addWorker( unit );
    }
  }

  void BaseManager::removeWorker( UnitRef unit )
  {
    for ( auto& base : bases_ )
      base.remove( unit );
  }

  BaseVector& BaseManager::bases()
  {
    return bases_;
  }

}