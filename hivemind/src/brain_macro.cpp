#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"

namespace hivemind {

  namespace Goals {



    Brain_Macro::Brain_Macro( AI::Agent * agent ):
      AI::CompositeGoal( agent )
    {
      bot_->messaging().listen( Listen_Global, this );

      // Hack for startup buildings.
      auto& hatcheryState = unitStats_[sc2::UNIT_TYPEID::ZERG_HATCHERY];
      hatcheryState.unitCount = 1;
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
      if (msg.code == M_Global_ConstructionCompleted || msg.code == M_Global_UnitCreated)
      {
        UnitRef unit = msg.unit();
        if ( !utils::isMine( unit ) )
          return;

        if( utils::isBuilding(unit) && msg.code == M_Global_UnitCreated)
          return;

        auto& stats = unitStats_[unit->unit_type];
        ++stats.unitCount;

        if(stats.inProgressCount > 0) // TODO: this is a hack for the startup units.
        {
          --stats.inProgressCount; // TODO: Human commands mess this up.
        }
      }
      else if(msg.code == M_Global_UnitDestroyed)
      {
        UnitRef unit = msg.unit();
        if ( !utils::isMine( unit ) )
          return;

        auto& stats = unitStats_[unit->unit_type];
        --stats.unitCount;
        // TODO: destroyed building might have been in progress.
        // TODO: zerg egg might have been in progress.
        // TODO: destroyed building might have had other units in progress.
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

      auto& poolState = unitStats_[sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL];
      auto& overlordState = unitStats_[sc2::UNIT_TYPEID::ZERG_OVERLORD];
      auto& extractorState = unitStats_[sc2::UNIT_TYPEID::ZERG_EXTRACTOR];
      auto& hatcheryState = unitStats_[sc2::UNIT_TYPEID::ZERG_HATCHERY];
      auto& droneState = unitStats_[sc2::UNIT_TYPEID::ZERG_DRONE];

      int futureSupplyLimit = std::min(200, supplyLimit + overlordState.inProgressCount * 8);
      int overlordNeed = (usedSupply >= futureSupplyLimit - 1 ? 1 : 0);

      int scoutDroneCount = 1;
      int futureDroneCount = droneState.futureCount();
      int droneNeed = futureDroneCount < hatcheryState.unitCount * (16 + 2 * 3) + 1 + scoutDroneCount;

      int futureExtractorCount = extractorState.futureCount();
      int extractorNeed = futureExtractorCount < hatcheryState.unitCount * 2 && futureDroneCount > hatcheryState.unitCount * 16 + futureExtractorCount * 3 + 1 + scoutDroneCount;

      int poolNeed = poolState.futureCount() == 0 && futureDroneCount >= 16 + 1 + scoutDroneCount;

      //bot_->console().printf( "minerals left: %d, allocated minerals %d", minerals, allocatedResourcesUnits.first );

      if(poolNeed > 0)
      {
        if(minerals > 200)
        {
          BuildProjectID id;

          auto buildingType = sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL;
          bool started = builder.add( buildingType, base, BuildPlacement_Generic, id);

          if(started)
          {
            ++poolState.inProgressCount;
          }
        }
      }
      else if(overlordNeed > 0)
      {
        if(minerals > 100)
        {
          BuildProjectID id;

          bool started = trainer.add(sc2::UNIT_TYPEID::ZERG_OVERLORD, base, sc2::UNIT_TYPEID::ZERG_LARVA, sc2::ABILITY_ID::TRAIN_OVERLORD, id);

          auto& unitState = unitStats_[sc2::UNIT_TYPEID::ZERG_OVERLORD];
          if(started)
          {
            unitState.inProgressCount += 1;
          }
        }
      }
      else if(extractorNeed > 0)
      {
        if(minerals > 50)
        {
          BuildProjectID id;

          auto buildingType = sc2::UNIT_TYPEID::ZERG_EXTRACTOR;
          bool started = builder.add( buildingType, base, BuildPlacement_Extractor, id);

          auto& unitState = unitStats_[buildingType];
          if(started)
          {
            unitState.inProgressCount += 1;
          }
        }
      }
      else if(droneNeed > 0)
      {
        if(minerals >= 50 && usedSupply < supplyLimit)
        {
          BuildProjectID id;

          bool started = trainer.add(sc2::UNIT_TYPEID::ZERG_DRONE, base, sc2::UNIT_TYPEID::ZERG_LARVA, sc2::ABILITY_ID::TRAIN_DRONE, id);

          auto& unitState = unitStats_[sc2::UNIT_TYPEID::ZERG_DRONE];
          if(started)
          {
            unitState.inProgressCount += 1;
          }
        }
      }
      else if(minerals >= 50 && usedSupply < supplyLimit && poolState.unitCount > 0)
      {
        BuildProjectID id;

        bool started = trainer.add(sc2::UNIT_TYPEID::ZERG_ZERGLING, base, sc2::UNIT_TYPEID::ZERG_LARVA, sc2::ABILITY_ID::TRAIN_ZERGLING, id);

        auto& unitState = unitStats_[sc2::UNIT_TYPEID::ZERG_ZERGLING];
        if(started)
        {
          unitState.inProgressCount += 2;
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
