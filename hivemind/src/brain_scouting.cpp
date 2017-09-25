#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Worker Scout goal */

    size_t Brain_WorkerScout::lostScouts_ = 0;
    GameTime Brain_WorkerScout::lostScoutTime_ = 0;

    Brain_WorkerScout::Brain_WorkerScout( AI::Agent* agent ):
    AI::CompositeGoal( agent )
    {
      bot_->messaging().listen( Listen_Global | Listen_Intel, this );
      activate();
    }

    Brain_WorkerScout::~Brain_WorkerScout()
    {
      terminate();
      bot_->messaging().remove( this );
    }

    void Brain_WorkerScout::activate()
    {
      status_ = AI::Goal::Status_Active;
      worker_ = 0;

      unexploredStartLocations_.clear();
      for ( auto& location : bot_->map().baseLocations_ )
        if ( location.isStartLocation() )
          unexploredStartLocations_.insert( location.baseID_ );

      exploredStartLocations_.clear();
      route_.clear();
      routeIndex_ = -1;
    }

    void Brain_WorkerScout::onMessage( const Message & msg )
    {
      if ( worker_ && msg.code == M_Global_UnitDestroyed && msg.unit()->tag == worker_ )
      {
        lostScouts_++;
        lostScoutTime_ = bot_->time();
        worker_ = 0;
      }
      else if ( msg.code == M_Intel_FoundPlayer )
      {
        _foundPlayer( msg.player( 0 ), msg.unit( 1 ) );
      }
    }

    void Brain_WorkerScout::_foundPlayer( PlayerID player, const Unit* unit )
    {
      bot_->console().printf( "WorkerScout: Found player %llu (%s)", player, sc2::UnitTypeToName( unit->unit_type ) );

      for ( auto& locId : unexploredStartLocations_ )
        if ( bot_->map().baseLocations_[locId].containsPosition( unit->pos ) )
        {
          unexploredStartLocations_.erase( locId );
          exploredStartLocations_.insert( locId );
          break;
        }

      _replanRoute();
    }

    void Brain_WorkerScout::_sendWorker()
    {
      bot_->console().printf( "WorkerScout: Sending worker %llu", worker_ );

      _routeAdvance();
    }

    void Brain_WorkerScout::_routeAdvance()
    {
      routeIndex_++;
      if ( routeIndex_ < route_.size() )
      {
        bot_->console().printf( "WorkerScout: Navigating to next route node %llu", routeIndex_ );
        bot_->action().UnitCommand( worker_, sc2::ABILITY_ID::MOVE, route_[routeIndex_] );
      }
      else
      {
        bot_->console().printf( "WorkerScout: Route done, nothing to do" );
      }
    }

    void Brain_WorkerScout::_replanRoute()
    {
      route_.clear();

      bot_->console().printf( "WorkerScout: Replanning worker scout route" );

      if ( worker_ == 0 )
      {
        bot_->console().printf( "WorkerScout: Aborting route plan, no worker assigned" );
        return;
      }

      bool undiscoveredPlayers = false;
      for ( auto& enemy : bot_->intelligence().enemies() )
        if ( enemy.second.alive_ && enemy.second.lastSeen_ == 0 )
        {
          undiscoveredPlayers = true;
          break;
        }

      auto unit = bot_->observation().GetUnit( worker_ );

      if ( undiscoveredPlayers && !unexploredStartLocations_.empty() )
      {
        if ( route_.empty() || routeIndex_ == route_.size() )
        {
          vector<Vector2> locations;
          for ( auto locId : unexploredStartLocations_ )
            locations.emplace_back( bot_->map().baseLocations_[locId].getPosition() );

          routeIndex_ = -1;
          route_ = bot_->map().shortestScoutPath( unit->pos, locations );

          bot_->console().printf( "WorkerScout: Route planned with %llu nodes", route_.size() );

          _sendWorker();
        }
        else
        {
          _routeAdvance();
        }
      }
      else
      {
        bot_->console().printf( "WorkerScout: Don't know how to scout with no undiscovered players or unexplored start locations left" );
        route_.clear();
        routeIndex_ = -1;
      }
    }

    AI::Goal::Status Brain_WorkerScout::process()
    {
      if ( worker_ == 0 )
      {
        worker_ = bot_->workers().release();
        if ( worker_ )
          _replanRoute();
      }
      else if ( !route_.empty() && routeIndex_ < route_.size() )
      {
        auto unit = bot_->observation().GetUnit( worker_ );
        if ( route_[routeIndex_].distance( unit->pos ) < 5.0f )
        {
          bot_->console().printf( "WorkerScout: Reached route node %llu, nothing here" );
          for ( auto& locId : unexploredStartLocations_ )
            if ( bot_->map().baseLocations_[locId].containsPosition( unit->pos ) )
            {
              unexploredStartLocations_.erase( locId );
              exploredStartLocations_.insert( locId );
              break;
            }
          _routeAdvance();
        }
      }
      return status_;
    }

    void Brain_WorkerScout::terminate()
    {
      bot_->console().printf( "WorkerScout::terminate" );
      status_ = AI::Goal::Status_Inactive;
      if ( worker_ )
      {
        bot_->workers().add( worker_ );
        worker_ = 0;
      }
    }

    /* Brain: Scout Enemy goal */

    class WorkerScoutEvaluator: public AI::GoalEvaluator {
    public:
      const Real calculateDesirability( AI::Agent* agent ) final
      {
        return ( ((hivemind::Brain*)agent)->bot_->timeSeconds() < 120 ? 1.0f : 0.1f );
      }
      void apply( AI::Goal* to, AI::Agent* agent ) final
      {
        to->addSubgoal( new Brain_WorkerScout( agent ) );
      }
    };

    Brain_ScoutEnemy::Brain_ScoutEnemy( AI::Agent* agent ):
    AI::ThinkGoal( agent )
    {
      evaluators_.push_back( new WorkerScoutEvaluator() );
    }

    void Brain_ScoutEnemy::activate()
    {
      AI::ThinkGoal::activate();
    }

    AI::Goal::Status Brain_ScoutEnemy::process()
    {
      return AI::ThinkGoal::process();
    }

    void Brain_ScoutEnemy::terminate()
    {
      AI::ThinkGoal::terminate();
    }

  }

}