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

    /* Brain: Root goal */

    Brain_Root::Brain_Root( AI::Agent* agent ):
    AI::GoalCollection( agent )
    {
    }

    Brain_Root::~Brain_Root()
    {
    }

    void Brain_Root::activate()
    {
      scoutGoal_ = new Brain_ScoutEnemy( owner_ );
      economyGoal_ = new Brain_ManageEconomy( owner_ );
      microGoal_ = new Brain_Micro( owner_ );
      goalList_.push_back( scoutGoal_ );
      goalList_.push_back( economyGoal_ );
      goalList_.push_back( microGoal_ );
    }

    void Brain_Root::terminate()
    {
      delete scoutGoal_;
      delete economyGoal_;
      delete microGoal_;
      goalList_.clear();
    }

    void Brain_Root::evaluate()
    {
      scoutGoal_->setImportance( 1.0f );
      economyGoal_->setImportance( 0.9f );
      microGoal_->setImportance( 0.5f );
    }

  }

  Brain::Brain( Bot* bot ): Subsystem( bot ), goals_( this )
  {
  }

  void Brain::gameBegin()
  {
    goals_.activate();
  }

  void Brain::gameEnd()
  {
    goals_.terminate();
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