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
    
    /* Brain: Build goal */

    Brain_Build::Brain_Build( AI::Agent * agent ):
      AI::CompositeGoal( agent )
    {
    }

    void Brain_Build::activate()
    {
      status_ = AI::Goal::Status_Active;
    }

    AI::Goal::Status Brain_Build::process()
    {
      static int poolCount = 0;

      int minerals = bot_->Observation()->GetMinerals();
      auto& baseManager = bot_->bases();
      auto& builder = bot_->builder();
      auto& trainer = bot_->trainer();

      if(baseManager.bases().empty())
      {
        return status_;
      }
      auto& base = baseManager.bases().back();

      if(poolCount == 0)
      {
        if(minerals >= 200)
        {
          BuildProjectID id;
          builder.add(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, base, sc2::ABILITY_ID::BUILD_SPAWNINGPOOL, id);

          ++poolCount;
        }
      }
      else
      {
        if(minerals >= 50)
        {
          BuildProjectID id;

          trainer.add(sc2::UNIT_TYPEID::ZERG_ZERGLING, base, sc2::UNIT_TYPEID::ZERG_LARVA, sc2::ABILITY_ID::TRAIN_ZERGLING, id);
        }
      }

      return status_;
    }

    void Brain_Build::terminate()
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
      buildGoal_ = new Brain_Build( owner_ );

      goalList_.push_back( harvestersGoal_ );
      goalList_.push_back( creepGoal_ );
      goalList_.push_back( buildGoal_ );

      harvestersGoal_->activate();
      creepGoal_->activate();
      buildGoal_->activate();
    }

    void Brain_ManageEconomy::terminate()
    {
      harvestersGoal_->terminate();
      delete harvestersGoal_;
      creepGoal_->terminate();
      delete creepGoal_;
      buildGoal_->terminate();
      delete buildGoal_;

      goalList_.clear();
    }

    void Brain_ManageEconomy::evaluate()
    {
      harvestersGoal_->setImportance( 1.0f );
      creepGoal_->setImportance( 0.7f );
      buildGoal_->setImportance( 0.6f );
    }

  }

}