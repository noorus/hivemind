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
      Tag worker_;
      set<size_t> unexploredStartLocations_;
      set<size_t> exploredStartLocations_;
      Path route_;
      size_t routeIndex_;
      void _foundPlayer( PlayerID player, const Unit* unit );
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

    class Brain_UpdateHarvesters: public AI::CompositeGoal {
    public:
      virtual const string& getName() const final { static string name = "Brain_UpdateHarvesters"; return name; }
    public:
      Brain_UpdateHarvesters( AI::Agent* agent );
      virtual void activate() final;
      virtual Status process() final;
      virtual void terminate() final;
    };

    class Brain_ManageEconomy: public AI::GoalCollection {
    protected:
      Brain_UpdateHarvesters* harvestersGoal_;
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