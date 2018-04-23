#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "ai_goals.h"
#include "ai_agent.h"
#include "messaging.h"
#include "hive_vector2.h"

#include <unordered_set>

namespace hivemind {

  class Bot;

  namespace Goals {

    class Brain_SpreadCreep: public AI::CompositeGoal {
    protected:
      GameTime nextCreepTime_;
    public:
      virtual const string& getName() const final { static string name = "Brain_SpreadCreep"; return name; }
    public:
      Brain_SpreadCreep( AI::Agent* agent );
      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;
    };

  }

}
