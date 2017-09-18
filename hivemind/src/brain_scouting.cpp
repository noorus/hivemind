#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Worker Scout goal */

    Brain_WorkerScout::Brain_WorkerScout( AI::Agent* agent ):
    AI::CompositeGoal( agent )
    {
    }

    void Brain_WorkerScout::activate()
    {
      status_ = AI::Goal::Status_Active;
    }

    AI::Goal::Status Brain_WorkerScout::process()
    {
      return status_;
    }

    void Brain_WorkerScout::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

    /* Brain: Scout Enemy goal */

    class WorkerScoutEvaluator: public AI::GoalEvaluator {
    public:
      const Real calculateDesirability( AI::Agent* agent ) final
      {
        return 1.0f;
      }
      void apply( AI::Goal* to, AI::Agent* agent ) final
      {
        to->addSubgoal( new Brain_WorkerScout( agent ) );
      }
    };

    Brain_ScoutEnemy::Brain_ScoutEnemy( AI::Agent* agent ):
    AI::ThinkGoal( agent )
    {
      mEvaluators.push_back( new WorkerScoutEvaluator() );
    }

    void Brain_ScoutEnemy::activate()
    {
      AI::ThinkGoal::activate();
    }

    AI::Goal::Status Brain_ScoutEnemy::process()
    {
      return status_;
    }

    void Brain_ScoutEnemy::terminate()
    {
      AI::ThinkGoal::terminate();
    }

  }

}