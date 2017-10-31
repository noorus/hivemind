#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Micro goal */

    Brain_Micro::Brain_Micro( AI::Agent * agent ):
      AI::CompositeGoal( agent )
    {
    }

    void Brain_Micro::activate()
    {
      status_ = AI::Goal::Status_Active;
    }

    AI::Goal::Status Brain_Micro::process()
    {
      return status_;
    }

    void Brain_Micro::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

  }

}
