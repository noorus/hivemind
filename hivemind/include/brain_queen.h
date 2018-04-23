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

    class Brain_Queen: public AI::CompositeGoal, public hivemind::Listener {

    protected:
      GameTime nextUpdateTime_;

      struct InjectTask
      {
        Base* base;
        UnitRef hatchery;
        UnitRef queen;
      };

      std::vector<InjectTask> injects_;

      UnitRef acquireInjectorQueen(Base* base, UnitRef hatchery);

      void processLarvaInjections();
      void processCreepTumorBuilds();

    public:
      virtual const string& getName() const final { static string name = "Brain_Queen"; return name; }
      virtual void onMessage( const Message& msg ) final;

    public:
      Brain_Queen( AI::Agent* agent );
      ~Brain_Queen();

      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;
    };

  }

}
