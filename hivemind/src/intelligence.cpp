#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"

namespace hivemind {

  Intelligence::Intelligence( Bot* bot ): Subsystem( bot )
  {
  }

  void Intelligence::gameBegin()
  {
    bot_->messaging().listen( Listen_Global, this );

    enemies_.clear();
    units_.clear();

    for ( auto& player : bot_->players().all() )
    {
      if ( player.isEnemy() )
      {
        bot_->console().printf( "Intelligence: Init enemy %llu", player.id() );
        enemies_[player.id()].alive_ = true;
        enemies_[player.id()].lastSeen_ = 0;
      }
    }
  }

  void Intelligence::update( const GameTime time, const GameTime delta )
  {
  }

  void Intelligence::onMessage( const Message& msg )
  {
    if ( msg.code == M_Global_UnitEnterVision && utils::isEnemy( msg.unit() ) )
    {
      auto enemy = &enemies_[msg.unit()->owner];
      if ( enemy->lastSeen_ == 0 )
      {
        auto closestBase = bot_->map().closestLocation(msg.unit()->pos);
        bool closestBaseIsStartLocation = closestBase && closestBase->isStartLocation();

        if ( closestBaseIsStartLocation || utils::isMainStructure( msg.unit() ) )
        {
          bot_->messaging().sendGlobal( M_Intel_FoundPlayer, 2, (size_t)msg.unit()->owner, msg.unit() );
          // TODO separate baseSeen value etc.
          enemy->lastSeen_ = bot_->time();
        }
      }
    }
  }

  void Intelligence::gameEnd()
  {
    bot_->messaging().remove( this );
  }

}