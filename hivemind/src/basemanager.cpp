#include "stdafx.h"
#include "basemanager.h"
#include "bot.h"
#include "exception.h"
#include "utilities.h"

namespace hivemind {

  BaseManager::BaseManager( Bot * bot ): Subsystem( bot )
  {
  }

  bool BaseManager::addBase( const Unit& depot )
  {
    BaseLocation* location = nullptr;
    for ( auto& it : bot_->map().baseLocations_ )
      if ( it.containsPosition( depot.pos ) )
      {
        location = &it;
        break;
      }

    if ( !location )
      return false;

    bot_->console().printf( "Adding base (%s) at location %llu", sc2::UnitTypeToName( depot.unit_type ), location->baseID_ );

    bases_.emplace_back( bases_.size(), location, depot );

    return true;
  }

  void BaseManager::gameBegin()
  {
    // add initial base here because we can't know the order of initial creation events for adding units to the base.
    for ( auto& unit : bot_->observation().GetUnits( sc2::Unit::Alliance::Self ) )
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
  }

  void BaseManager::draw()
  {
    for ( auto& base : bases_ )
    {
      char asd[128];
      sprintf_s( asd, 128, "Base %llu\nWorkers %llu\nQueens %llu\nBuildings %llu", base.id(), base.workers().size(), base.queens().size(), base.buildings().size() );
      bot_->debug().DebugTextOut( asd, Point3D( base.location()->position_.x, base.location()->position_.y, bot_->map().maxZ_ + 0.1f ), sc2::Colors::Purple );
    }
  }

  void BaseManager::onMessage( const Message & msg )
  {
    if ( msg.code == M_Global_UnitCreated && utils::isMine( *msg.unit() ) )
    {
      // Hack to avoid double events in the beginning.
      if ( assignedUnits_.find( msg.unit()->tag ) != assignedUnits_.end() )
        return;
      assignedUnits_.insert( msg.unit()->tag );

      if ( utils::isBuilding( *msg.unit() ) )
      {
        bot_->console().printf( "Building created: %s", sc2::UnitTypeToName( (*msg.unit()).unit_type ) );
        addBuilding( *msg.unit() );
      }
      else
      {
        auto base = findClosest( msg.unit()->pos );
        if ( msg.unit()->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_LARVA && base )
        {
          bot_->console().printf( "Base %llu adding larva %llu", base->id(), msg.unit()->tag );
          base->addLarva( msg.unit()->tag );
        }
        else if ( msg.unit()->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_DRONE && base )
        {
          // bot_->console().printf( "Base %llu adding drone %llu", base->id(), msg.unit()->tag );
          // base->addQueen( msg.unit()->tag );
        }
        else if ( msg.unit()->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_QUEEN && base )
        {
          bot_->console().printf( "Base %llu adding queen %llu", base->id(), msg.unit()->tag );
          base->addQueen( msg.unit()->tag );
        }
      }
    }
    else if ( msg.code == M_Global_UnitDestroyed && utils::isMine( *msg.unit() ) )
    {
      assignedUnits_.erase( msg.unit()->tag );
      for ( auto& base : bases_ )
        base.onDestroyed( *msg.unit() );
    }
  }

  Base* BaseManager::findClosest( const Vector3 & pos )
  {
    Base* ret = nullptr;
    Real minDist = 32000.0f;
    for ( auto& base : bases_ )
    {
      auto dist = base.location()->getPosition().squaredDistance( pos.to2() );
      if ( dist < minDist )
      {
        minDist = dist;
        ret = &base;
      }
    }

    return ret;
  }

  void BaseManager::addBuilding( const Unit& building )
  {
    Base* base = nullptr;
    if ( utils::isMainStructure( building ) )
    {
      // if new main structure is inside an existing base's area, add it to that base.
      // otherwise if building is inside a base location's area, create a new base with that location.
      // otherwise just add it to whatever is the closest base.
      // TODO Find base by polygonal region rather than distance, distance throws off way too easily.
      depots_.insert( building.tag );
      for ( auto& it : bases_ )
        if ( it.location()->containsPosition( building.pos ) )
          base = &it;
      if ( base )
        base->addDepot( building );
      else
      {
        if ( !addBase( building ) )
        {
          base = findClosest( building.pos );
          if ( base )
            base->addDepot( building );
        }
      }
    }
    else
    {
      base = findClosest( building.pos );
      if ( base )
        base->addBuilding( building );
    }
  }

  BaseVector& BaseManager::bases()
  {
    return bases_;
  }

}