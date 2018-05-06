#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "ai_goals.h"
#include "ai_agent.h"
#include "messaging.h"
#include "hive_vector2.h"

#include <unordered_set>

#include <boost/optional/optional.hpp>


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

      struct TumorTask
      {
        Base* base;
        UnitRef queen;
        Vector2 tumorLocation;
      };

      std::vector<InjectTask> injectTasks_;
      std::vector<TumorTask> tumorTasks_;

      UnitRef acquireInjectorQueen(Base* base, UnitRef hatchery);
      UnitRef acquireTumorQueen(Base* base);

      boost::optional<Vector2> chooseTumorLocation(Base* base, UnitRef queen) const;

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
