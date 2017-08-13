#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  const char* c_goalStatusTexts[4] = {
    "Active",
    "Inactive",
    "Completed",
    "Failed"
  };

  namespace Goals {

    /* Brain: Worker Scout goal */

    Brain_WorkerScout::Brain_WorkerScout( AI::Agent* agent ):
    AI::CompositeGoal( agent )
    {
    }

    void Brain_WorkerScout::activate()
    {
    }

    AI::Goal::Status Brain_WorkerScout::process()
    {
      return Status();
    }

    void Brain_WorkerScout::terminate()
    {
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
    }

    AI::Goal::Status Brain_ScoutEnemy::process()
    {
      return Status();
    }

    void Brain_ScoutEnemy::terminate()
    {
    }

    /* Brain: Manage Economy goal */

    Brain_ManageEconomy::Brain_ManageEconomy( AI::Agent* agent ):
    AI::GoalCollection( agent )
    {
    }

    void Brain_ManageEconomy::activate()
    {
    }

    void Brain_ManageEconomy::terminate()
    {
    }

    void Brain_ManageEconomy::evaluate()
    {
    }

    /* Brain: Root goal */

    Brain_Root::Brain_Root( AI::Agent* agent ):
    AI::GoalCollection( agent )
    {
      activate();
    }

    Brain_Root::~Brain_Root()
    {
      terminate();
    }

    void Brain_Root::activate()
    {
      scoutGoal_ = new Brain_ScoutEnemy( owner_ );
      economyGoal_ = new Brain_ManageEconomy( owner_ );
      goalList_.push_back( scoutGoal_ );
      goalList_.push_back( economyGoal_ );
    }

    void Brain_Root::terminate()
    {
      delete scoutGoal_;
      delete economyGoal_;
      goalList_.clear();
    }

    void Brain_Root::evaluate()
    {
      scoutGoal_->setImportance( 1.0f );
      economyGoal_->setImportance( 0.9f );
    }

  }

  Brain::Brain( Bot* bot ): Subsystem( bot ), goals_( this )
  {
  }

  void Brain::gameBegin()
  {
  }

  void Brain::gameEnd()
  {
  }

  void Brain::update( const GameTime time, const GameTime delta )
  {
    goals_.evaluate();
    goals_.process();
  }

  void debugDrawGoalTree( const string& title, AI::Goal* goal, Point2D screenPosition, sc2::DebugInterface& dbg )
  {
    const Point2D increment( 0.005f, 0.01f );
    dbg.DebugTextOut( title, screenPosition, sc2::Colors::White );
    screenPosition.y += increment.y;

    std::function<void( Point2D&, AI::Goal* )> recurse = [&]( Point2D& pos, AI::Goal* goal )
    {
      auto color = ( goal->isCollection() ? sc2::Colors::Purple : goal->isComposite() ? sc2::Colors::Green : sc2::Colors::Teal );

      char text[128];
      sprintf_s( text, 128, "%s (%s)", goal->getName().c_str(), c_goalStatusTexts[goal->getStatus()] );
      dbg.DebugTextOut( text, pos, color );

      pos.y += increment.y;
      if ( goal->isComposite() )
      {
        pos.x += increment.x;
        auto comp = static_cast<AI::CompositeGoal*>( goal );
        for ( auto it : comp->getSubgoals() )
          recurse( pos, it );
        pos.x -= increment.x;
      }
      else if ( goal->isCollection() )
      {
        pos.x += increment.x;
        auto comp = static_cast<AI::GoalCollection*>( goal );
        for ( auto it : comp->getGoals() )
          recurse( pos, it );
        pos.x -= increment.x;
      }
    };
    recurse( screenPosition, goal );
  }

  void Brain::draw()
  {
    debugDrawGoalTree( "BRAIN", &goals_, Point2D( 0.03f, 0.125f ), bot_->debug() );
  }

}