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
  class BuildPlanner;

  namespace Goals {

    class Brain_Macro: public AI::CompositeGoal, public hivemind::Listener {
    public:
      virtual const string& getName() const final { static string name = "Brain_Macro"; return name; }
      virtual void onMessage( const Message& msg ) final;

    public:
      Brain_Macro( AI::Agent* agent );
      ~Brain_Macro();

      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;

      void update();

    private:
      std::unique_ptr<BuildPlanner> planner_;
    };

  }

}
