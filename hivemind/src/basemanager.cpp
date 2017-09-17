#include "stdafx.h"
#include "basemanager.h"
#include "bot.h"
#include "exception.h"
#include "utilities.h"

namespace hivemind {

  BaseManager::BaseManager( Bot * bot ): Subsystem( bot )
  {
  }

  void BaseManager::gameBegin()
  {
    BaseLocation* myLocation = nullptr;
    for ( auto& loc : bot_->map().baseLocations_ )
      if ( loc.isPlayerStartLocation( bot_->players().self().id() ) )
        myLocation = &loc;

    if ( !myLocation )
      HIVE_EXCEPT( "Can't determine my start location" );

    UnitMap buildings;
    for ( auto& unit : bot_->observation().GetUnits() )
      if ( utils::isMine( unit ) && utils::isBuilding( unit ) && myLocation->containsPosition( unit.pos ) )
        buildings[unit.tag] = unit.unit_type;

    bases_.emplace_back( myLocation, bot_->workers().all(), buildings );
  }

  void BaseManager::gameEnd()
  {
    bases_.clear();
  }

  void BaseManager::update( const GameTime time, const GameTime delta )
  {
  }

  void BaseManager::draw()
  {
  }

  BaseVector& BaseManager::bases()
  {
    return bases_;
  }

}