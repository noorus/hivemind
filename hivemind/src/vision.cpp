#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "vision.h"

namespace hivemind {

  Vision::Vision( Bot* bot ): Subsystem( bot )
  {
  }

  void Vision::gameBegin()
  {
    bot_->messaging().listen( Listen_Global | Listen_Intel, this );
  }

  void Vision::update( const GameTime time, const GameTime delta )
  {
  }

  void Vision::onMessage( const Message & msg )
  {
    if ( msg.code == M_Intel_FoundPlayer )
    {
      for ( auto& locId : bot_->vision().startLocations().unexplored_ )
        if ( bot_->map().baseLocations_[locId].containsPosition( msg.unit( 1 )->pos ) )
        {
          bot_->vision().startLocations().unexplored_.erase( locId );
          bot_->vision().startLocations().explored_.insert( locId );
          break;
        }
    }
  }

  void Vision::gameEnd()
  {
    bot_->messaging().remove( this );
  }

}