#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {



    Brain_Macro::Brain_Macro( AI::Agent * agent ):
      AI::CompositeGoal( agent )
    {
      bot_->messaging().listen( Listen_Global, this );
    }

    Brain_Macro::~Brain_Macro()
    {
      bot_->messaging().remove(this);
    }


    void Brain_Macro::activate()
    {
      status_ = AI::Goal::Status_Active;
    }

    void Brain_Macro::onMessage( const Message& msg )
    {
      if ( msg.code == M_Global_ConstructionCompleted )
      {
        UnitRef unit = msg.unit();
        if ( !utils::isMine( unit ) )
          return;
        if ( !utils::isBuilding( unit ) )
          return;

        techState_[unit->unit_type] = Ready;
      }
    }

    AI::Goal::Status Brain_Macro::process()
    {
      if(!isActive())
      {
        return status_;
      }

      int minerals = bot_->Observation()->GetMinerals();
      int vespene = bot_->Observation()->GetVespene();
      int supplyLimit = bot_->Observation()->GetFoodCap();
      int usedSupply = bot_->Observation()->GetFoodUsed();

      auto& baseManager = bot_->bases();
      auto& builder = bot_->builder();
      auto& trainer = bot_->trainer();

      std::pair<int,int> allocatedResourcesBuildings = builder.getAllocatedResources();
      minerals -= allocatedResourcesBuildings.first;
      vespene -= allocatedResourcesBuildings.second;

      std::pair<int,int> allocatedResourcesUnits = trainer.getAllocatedResources();
      minerals -= allocatedResourcesUnits.first;
      vespene -= allocatedResourcesUnits.second;

      if(baseManager.bases().empty())
      {
        return status_;
      }
      auto& base = baseManager.bases().back();

      auto& poolState = techState_[sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL];

      //bot_->console().printf( "minerals left: %d, allocated minerals %d", minerals, allocatedResourcesUnits.first );

      if(poolState == Missing)
      {
        if(minerals >= 200)
        {
          BuildProjectID id;
          builder.add(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, base, sc2::ABILITY_ID::BUILD_SPAWNINGPOOL, id);

          poolState = InProgress;
        }
      }
      else
      {
        if(minerals >= 50 && usedSupply < supplyLimit && poolState == Ready)
        {
          BuildProjectID id;

          trainer.add(sc2::UNIT_TYPEID::ZERG_ZERGLING, base, sc2::UNIT_TYPEID::ZERG_LARVA, sc2::ABILITY_ID::TRAIN_ZERGLING, id);
        }
        else if(minerals >= 100 && supplyLimit < 200 && usedSupply >= supplyLimit - 1)
        {
          BuildProjectID id;

          trainer.add(sc2::UNIT_TYPEID::ZERG_OVERLORD, base, sc2::UNIT_TYPEID::ZERG_LARVA, sc2::ABILITY_ID::TRAIN_OVERLORD, id);
        }
        else if(minerals >= 50 && usedSupply < supplyLimit)
        {
          BuildProjectID id;

          trainer.add(sc2::UNIT_TYPEID::ZERG_DRONE, base, sc2::UNIT_TYPEID::ZERG_LARVA, sc2::ABILITY_ID::TRAIN_DRONE, id);
        }
      }

      return status_;
    }

    void Brain_Macro::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

  }

}
