#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "ai_goals.h"
#include "ai_agent.h"
#include "messaging.h"
#include "hive_vector2.h"

namespace hivemind {

  class Bot;

  namespace Goals {

    class Brain_WorkerScout: public AI::CompositeGoal, public hivemind::Listener {
    private:
      UnitRef worker_;
      set<size_t> unexploredStartLocations_;
      set<size_t> exploredStartLocations_;
      Path route_;
      size_t routeIndex_;
      void _foundPlayer( PlayerID player, UnitRef unit );
      void _routeAdvance();
      void _replanRoute();
      void _sendWorker();
    public:
      enum State {

      };
      static size_t lostScouts_;
      static GameTime lostScoutTime_;
    public:
      virtual const string& getName() const final { static string name = "Brain_WorkerScout"; return name; }
      virtual void onMessage( const Message& msg ) final;
    public:
      Brain_WorkerScout( AI::Agent* agent );
      virtual ~Brain_WorkerScout();
      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;
    };

    class Brain_ScoutEnemy: public AI::ThinkGoal {
    public:
      virtual const string& getName() const final { static string name = "Brain_ScoutEnemy"; return name; }
    public:
      Brain_ScoutEnemy( AI::Agent* agent );
      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;
    };

    class Brain_Micro: public AI::CompositeGoal, public hivemind::Listener {
    protected:
    public:

      struct UnitBrain
      {
        int commandCooldown_;
        bool hasMoveTarget_;
        Vector2 moveTarget_;

        explicit UnitBrain():
          commandCooldown_(0),
          hasMoveTarget_(false)
        {
        }
      };

      struct Squad
      {
        using UnitBrains = std::map<UnitRef, UnitBrain>;
        UnitBrains units_;

        UnitRef focusTarget_;
        Vector2 center_;
        float radius_;
        Vector2 moveTarget_;

        explicit Squad() :
          focusTarget_(),
          radius_()
        {
        }

        void updateCenter();
      };

      virtual const string& getName() const final { static string name = "Brain_Micro"; return name; }
    public:
      Brain_Micro( AI::Agent* agent );
      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;
      virtual void onMessage(const Message& msg) final;
    private:

      void drawSquad(Squad& squad);
      void drawSquadUnits(Squad& squad);

      void updateSquad(Squad& squad);
      void updateSquadUnits(Squad& squad);
      void updateSquadUnit(Squad& squad, UnitRef unit, UnitBrain& brain);

      UnitRef selectClosestTarget(Vector2 from);
      UnitRef selectWeakestTarget(Vector2 from, float range);

      UnitRef selectTarget(const Squad& squad, UnitRef unit);
      Vector2 getKitingPosition(UnitRef unit);

      void log(const char* format, const std::string& message);

      template<typename ...Args>
      void log(const char* format, Args... args);

      Squad combatUnits_;
    };

    class Brain_UpdateHarvesters: public AI::CompositeGoal {
    public:
      virtual const string& getName() const final { static string name = "Brain_UpdateHarvesters"; return name; }
    public:
      Brain_UpdateHarvesters( AI::Agent* agent );
      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;
    };

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

    class Brain_ManageEconomy: public AI::GoalCollection {
    protected:
      Brain_UpdateHarvesters* harvestersGoal_;
      Brain_SpreadCreep* creepGoal_; // TODO somewhere else
    public:
      virtual const string& getName() const final { static string name = "Brain_ManageEconomy"; return name; }
    public:
      Brain_ManageEconomy( AI::Agent* agent );
      virtual ~Brain_ManageEconomy();
      virtual void activate() final;
      virtual void terminate() final;
      virtual void evaluate() final;
    };

    class Brain_Root: public AI::GoalCollection {
    protected:
      Brain_ScoutEnemy* scoutGoal_;
      Brain_ManageEconomy* economyGoal_;
      Brain_Micro* microGoal_;
    public:
      virtual const string& getName() const final { static string name = "Brain_Root"; return name; }
    public:
      Brain_Root( AI::Agent* agent );
      virtual ~Brain_Root();
      virtual void activate() final;
      virtual void terminate() final;
      virtual void evaluate() final;
    };

  }

  class Brain: public Subsystem, public AI::Agent {
  protected:
    Goals::Brain_Root goals_;
  public:
    Brain( Bot* bot );
    AI::Goal* getRootGoal() { return &goals_; }
    void gameBegin() final;
    void gameEnd() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
  };

}