#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Update Harvesters goal */

    Brain_UpdateHarvesters::Brain_UpdateHarvesters( AI::Agent * agent ):
    AI::CompositeGoal( agent )
    {
    }

    void Brain_UpdateHarvesters::activate()
    {
      status_ = AI::Goal::Status_Active;
    }

    AI::Goal::Status Brain_UpdateHarvesters::process()
    {
      for ( auto& base : bot_->bases().bases() )
      {
        // base.saturation()
      }
      return status_;
    }

    void Brain_UpdateHarvesters::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

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
      goalList_.push_back( harvestersGoal_ );
      goalList_.push_back( creepGoal_ );
      harvestersGoal_->activate();
      creepGoal_->activate();
    }

    void Brain_ManageEconomy::terminate()
    {
      harvestersGoal_->terminate();
      delete harvestersGoal_;
      creepGoal_->terminate();
      delete creepGoal_;
      goalList_.clear();
    }

    void Brain_ManageEconomy::evaluate()
    {
      harvestersGoal_->setImportance( 1.0f );
      creepGoal_->setImportance( 0.7f );
    }

  }

}