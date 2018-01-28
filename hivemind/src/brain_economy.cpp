#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "brain_creep.h"
#include "brain_queen.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Manage Economy goal */

    Brain_ManageEconomy::Brain_ManageEconomy( AI::Agent* agent ):
    AI::GoalCollection( agent )
    {
      activate();
    }

    Brain_ManageEconomy::~Brain_ManageEconomy()
    {
      terminate();
    }

    void Brain_ManageEconomy::activate()
    {
      harvestersGoal_ = new Brain_UpdateHarvesters( owner_ );
      creepGoal_ = new Brain_SpreadCreep( owner_ );
      buildGoal_ = new Brain_Macro( owner_ );
      overlordGoal_ = new Brain_ManageOverlords( owner_ );
      queenGoal_ = new Brain_Queen( owner_ );

      goalList_.push_back( harvestersGoal_ );
      goalList_.push_back( creepGoal_ );
      goalList_.push_back( buildGoal_ );
      goalList_.push_back( overlordGoal_ );
      goalList_.push_back( queenGoal_ );

      harvestersGoal_->activate();
      creepGoal_->activate();
      buildGoal_->activate();
      overlordGoal_->activate();
      queenGoal_->activate();
    }

    void Brain_ManageEconomy::terminate()
    {
      harvestersGoal_->terminate();
      delete harvestersGoal_;
      creepGoal_->terminate();
      delete creepGoal_;
      buildGoal_->terminate();
      delete buildGoal_;
      overlordGoal_->terminate();
      delete overlordGoal_;
      queenGoal_->terminate();
      delete queenGoal_;

      goalList_.clear();
    }

    void Brain_ManageEconomy::evaluate()
    {
      harvestersGoal_->setImportance( 1.0f );
      creepGoal_->setImportance( 0.7f );
      buildGoal_->setImportance( 0.8f );
      overlordGoal_->setImportance( 0.5f );
      queenGoal_->setImportance( 0.5f );
    }

  }

}