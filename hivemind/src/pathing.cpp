#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"
#include "pathing.h"

namespace hivemind {

  Pathing::Pathing( Bot* bot ): Subsystem( bot )
  {
  }

  void Pathing::gameBegin()
  {
    bot_->messaging().listen( Listen_Global, this );
  }

  void Pathing::update( const GameTime time, const GameTime delta )
  {
  }

  void Pathing::draw()
  {
  }

  void Pathing::onMessage( const Message& msg )
  {
  }

  void Pathing::gameEnd()
  {
    bot_->messaging().remove( this );
  }

}