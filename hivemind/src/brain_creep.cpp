#include "stdafx.h"
#include "brain.h"
#include "brain_creep.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Spread creep goal */

    const GameTime cCreepCheckDelay = 250;

    Brain_SpreadCreep::Brain_SpreadCreep( AI::Agent * agent ):
    AI::CompositeGoal( agent )
    {
    }

    void Brain_SpreadCreep::activate()
    {
      status_ = AI::Goal::Status_Active;
      nextCreepTime_ = bot_->time();
    }

    AI::Goal::Status Brain_SpreadCreep::process()
    {
      return status_;
    }

    void Brain_SpreadCreep::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

  }

}
