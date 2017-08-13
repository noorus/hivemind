#include "stdafx.h"
#include "ai_goals.h"
#include "exception.h"

namespace hivemind {

  namespace AI {

    /* Goal class */

    void Goal::activateIfInactive()
    {
      if ( isInactive() )
        activate();
    }

    void Goal::reactivateIfFailed()
    {
      if ( hasFailed() )
        status_ = Status_Inactive;
    }

    Goal::Goal( Agent* owner ): owner_( owner ), status_( Status_Inactive ), importance_( 0.0f )
    {
      //
    }

    Goal::~Goal()
    {
      //
    }

    void Goal::addSubgoal( Goal* goal )
    {
      ENGINE_EXCEPT( "Cannot add a subgoal to an atomic goal" );
    }

    /* CompositeGoal class */

    Goal::Status CompositeGoal::processSubgoals()
    {
      while ( !subGoals_.empty() && ( subGoals_.front()->isComplete() || subGoals_.front()->hasFailed() ) )
      {
        auto goal = subGoals_.front();
        goal->terminate();
        delete goal;
        subGoals_.pop_front();
      }
      if ( subGoals_.empty() ) {
        status_ = Status_Completed;
        return status_;
      }
      status_ = subGoals_.front()->process();
      if ( status_ == Status_Completed && !subGoals_.empty() )
        status_ = Status_Active;
      return status_;
    }

    CompositeGoal::CompositeGoal( Agent* owner ): Goal( owner )
    {
      //
    }

    CompositeGoal::~CompositeGoal()
    {
      removeAllSubgoals();
    }

    void CompositeGoal::addSubgoal( Goal* goal )
    {
      subGoals_.push_front( goal );
    }

    void CompositeGoal::removeAllSubgoals()
    {
      for ( auto goal : subGoals_ )
      {
        goal->terminate();
        delete goal;
      }
      subGoals_.clear();
    }

    /* GoalCollection class */

    GoalCollection::GoalCollection( Agent* owner ): Goal( owner )
    {
    }

    Goal::Status GoalCollection::process()
    {
      goalList_.sort( []( const Goal* a, const Goal* b ) {
        return ( a->getImportance() > b->getImportance() );
      } );

      for ( auto goal : goalList_ )
        goal->process();

      status_ = Status_Active;
      return status_;
    }

    /* ThinkGoal class */

    ThinkGoal::ThinkGoal( Agent* owner ): CompositeGoal( owner )
    {
    }

    void ThinkGoal::arbitrate()
    {
      Real best = 0.0f;
      GoalEvaluator* mostDesirable = nullptr;
      for ( auto evaluator : mEvaluators )
      {
        Real desirability = evaluator->calculateDesirability( owner_ );
        if ( desirability >= best )
        {
          best = desirability;
          mostDesirable = evaluator;
        }
      }
      if ( !mostDesirable )
        ENGINE_EXCEPT( "No evaluator selected" );
      mostDesirable->apply( this, owner_ );
    }

    void ThinkGoal::activate()
    {
      arbitrate();
      status_ = Status_Active;
    }

    Goal::Status ThinkGoal::process()
    {
      activateIfInactive();
      auto subStatus = processSubgoals();
      if ( subStatus == Status_Completed || subStatus == Status_Failed )
        status_ = Status_Inactive;
      return status_;
    }

    void ThinkGoal::terminate()
    {
      //
    }

  }

}