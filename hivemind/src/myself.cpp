#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "myself.h"
#include "database.h"

namespace hivemind {

  MyStructure::MyStructure(): created_( 0 ), completed_( 0 ), destroyed_( 0 )
  {
  }

  Myself::Myself( Bot* bot ): Subsystem( bot )
  {
  }

  void Myself::gameBegin()
  {
    bot_->messaging().listen( Listen_Global, this );
  }

  void Myself::update( const GameTime time, const GameTime delta )
  {
  }

  void Myself::draw()
  {
  }

  void Myself::onMessage( const Message& msg )
  {
    if ( msg.code == M_Global_UnitCreated && utils::isMine( msg.unit() ) )
    {
      if ( utils::isStructure( msg.unit() ) )
      {
        auto& struc = structures_[msg.unit()];
        struc.created_ = bot_->time();
      }
    }
    else if ( msg.code == M_Global_UnitDestroyed && utils::isMine( msg.unit() ) )
    {
      if ( utils::isStructure( msg.unit() ) )
      {
        auto& struc = structures_[msg.unit()];
        struc.destroyed_ = bot_->time();
      }
    }
    else if ( msg.code == M_Global_ConstructionCompleted && utils::isMine( msg.unit() ) )
    {
      if ( utils::isStructure( msg.unit() ) )
      {
        auto& struc = structures_[msg.unit()];
        struc.completed_ = bot_->time();
      }
    }
  }

  void Myself::gameEnd()
  {
    bot_->messaging().remove( this );
  }

}