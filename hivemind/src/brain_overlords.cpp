#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"
#include "controllers.h"

namespace hivemind {

  namespace Goals {

    static GameTime lastTime;

    Brain_ManageOverlords::Brain_ManageOverlords( AI::Agent* agent ):
      AI::CompositeGoal( agent )
    {
      bot_->messaging().listen( Listen_Global | Listen_Intel, this );
      activate();
    }

    Brain_ManageOverlords::~Brain_ManageOverlords()
    {
      terminate();
      bot_->messaging().remove( this );
    }

    inline const bool isOverlord( UnitRef unit )
    {
      return ( utils::isMine( unit ) && unit->unit_type == UNIT_TYPEID::ZERG_OVERLORD );
    }

    void Brain_ManageOverlords::_add( UnitRef overlord )
    {
      if ( isOverlord( overlord ) )
        overlords_.insert( overlord );
    }

    void Brain_ManageOverlords::_remove( UnitRef overlord )
    {
      overlords_.erase( overlord );
    }

    void Brain_ManageOverlords::activate()
    {
      status_ = AI::Goal::Status_Active;

      overlords_.clear();
      lastTime = 0;

      auto units = bot_->observation().GetUnits( Unit::Self );
      for ( auto& unit : units )
      {
        _add( unit );
      }
    }

    void Brain_ManageOverlords::onMessage( const Message& msg )
    {
      if ( msg.code == M_Global_UnitCreated && utils::isMine( msg.unit() ) && isOverlord( msg.unit() ) )
      {
        if ( isOverlord( msg.unit() ) )
          _add( msg.unit() );
      }
      else if ( msg.code == M_Global_UnitDestroyed && utils::isMine( msg.unit() ) )
      {
        _remove( msg.unit() );
      }
    }

    AI::Goal::Status Brain_ManageOverlords::process()
    {
      for ( auto overlord : overlords_ )
      {
        if ( !isOverlord( overlord ) )
        {
          _remove( overlord );
          continue;
        }
      }
      if ( lastTime <= bot_->time() )
      {
        lastTime = bot_->time() + 1000;
        /*auto& chokes = bot_->map().chokepointSides_;
        for ( auto overlord : overlords_ )
        {
          auto it = chokes.cbegin();
          std::advance( it, utils::randomBetween( 0, (int)chokes.size() - 1 ) );

          auto& choke = ( *it ).second;

          auto midpoint = choke.side1 + ( ( choke.side1 - choke.side2 ) * 0.5f );
          Overlord( overlord ).move( midpoint );
        }*/
      }
      return status_;
    }

    void Brain_ManageOverlords::terminate()
    {
      status_ = AI::Goal::Status_Inactive;

      overlords_.clear();
    }

  }

}