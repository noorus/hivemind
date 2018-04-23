#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "ai_goals.h"
#include "ai_agent.h"
#include "messaging.h"
#include "hive_vector2.h"
#include "ai_fuzzy.h"

namespace hivemind {

  class Bot;

  enum Focus {
    Focus_Economic = 0,
    Focus_Aggressive,
    Focus_Defensive
  };

  class Strategy_Base {
  protected:
    Bot* bot_;
  public:
    explicit Strategy_Base( Bot* bot );
    virtual const string& getName() const = 0;
  };

  class Strategy_Economic: public Strategy_Base {
  protected:
  public:
    explicit Strategy_Economic( Bot* bot );
    virtual const string& getName() const final { static string name = "Strategy_Economic"; return name; }
  };

  class Strategy_Aggressive: public Strategy_Base {
  protected:
  public:
    explicit Strategy_Aggressive( Bot* bot );
    virtual const string& getName() const final { static string name = "Strategy_Aggressive"; return name; }
  };

  class Strategy_Defensive: public Strategy_Base {
  protected:
  public:
    explicit Strategy_Defensive( Bot* bot );
    virtual const string& getName() const final { static string name = "Strategy_Defensive"; return name; }
  };

  using StrategyVector = vector<Strategy_Base*>;

  class Strategy: public Subsystem {
  protected:
    StrategyVector strats_;
    AI::FuzzyModule fuzzyMod_;
    AI::FuzzyVariable* fzGameTime_;
  public:
    Strategy( Bot* bot );
    ~Strategy();
    void gameBegin() final;
    void gameEnd() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
  };

}