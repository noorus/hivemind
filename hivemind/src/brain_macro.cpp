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
    }

    AI::Goal::Status Brain_Macro::process()
    {
      if(!isActive())
      {
        return status_;
      }

      update();

      return status_;
    }

    void Brain_Macro::update()
    {
      int minerals = bot_->Observation()->GetMinerals();
      int vespene = bot_->Observation()->GetVespene();
      int supplyLimit = bot_->Observation()->GetFoodCap();
      int usedSupply = bot_->Observation()->GetFoodUsed();

      auto& baseManager = bot_->bases();
      auto& builder = bot_->builder();

      std::pair<int, int> allocatedResources = builder.getAllocatedResources();
      minerals -= allocatedResources.first;
      vespene -= allocatedResources.second;

      if(baseManager.bases().empty())
      {
        return;
      }

      auto& poolState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
      auto& overlordState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_OVERLORD);
      auto& extractorState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_EXTRACTOR);
      auto& hatcheryState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_HATCHERY);
      auto& droneState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_DRONE);
      auto& queenState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_QUEEN);

      int futureSupplyLimit = std::min(200, supplyLimit + overlordState.inProgressCount() * 8);
      int overlordNeed = (usedSupply >= futureSupplyLimit - 1 ? 1 : 0);

      int scoutDroneCount = 1;
      int futureDroneCount = droneState.futureCount();
      int droneNeed = futureDroneCount < hatcheryState.unitCount() * (16 + 2 * 3) + scoutDroneCount && futureDroneCount <= 75;

      int futureExtractorCount = extractorState.futureCount();
      int extractorNeed = futureExtractorCount < hatcheryState.unitCount() && futureDroneCount > hatcheryState.unitCount() * 16 + futureExtractorCount * 3 + 1 + scoutDroneCount;

      int poolNeed = poolState.futureCount() == 0 && futureDroneCount >= 16 + 1 + scoutDroneCount;

      int queenNeed = poolState.unitCount() > 0 && queenState.futureCount() < hatcheryState.unitCount() && queenState.inProgressCount() < hatcheryState.unitCount();

      int hatcheryNeed = droneState.futureCount() >= 17 * hatcheryState.futureCount();

      //bot_->console().printf( "minerals left: %d, allocated minerals %d", minerals, allocatedResources.first );

      BuildProjectID id;

      if(hatcheryNeed > 0)
      {
        if(minerals >= 300)
        {
          for(auto& base : baseManager.bases())
            if(builder.build(sc2::UNIT_TYPEID::ZERG_HATCHERY, &base, BuildPlacement_MainBuilding, id))
              return;
        }
      }
      else if(poolNeed > 0)
      {
        if(minerals >= 200)
        {
          for(auto& base : baseManager.bases())
            if(builder.build(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, &base, BuildPlacement_Generic, id))
              return;
        }
      }
      else if(overlordNeed > 0)
      {
        if(minerals >= 100)
        {
          for(auto& base : baseManager.bases())
            if(builder.train(sc2::UNIT_TYPEID::ZERG_OVERLORD, &base, sc2::UNIT_TYPEID::ZERG_LARVA, id))
              return;
        }
      }
      else if(queenNeed > 0)
      {
        if(minerals >= 150 && usedSupply + 1 < supplyLimit)
        {
          for(auto& base : baseManager.bases())
            if(builder.train(sc2::UNIT_TYPEID::ZERG_QUEEN, &base, sc2::UNIT_TYPEID::ZERG_HATCHERY, id))
              return;
        }
      }
      else if(extractorNeed > 0)
      {
        if(minerals >= 50)
        {
          for(auto& base : baseManager.bases())
            if(builder.build(sc2::UNIT_TYPEID::ZERG_EXTRACTOR, &base, BuildPlacement_Extractor, id))
              return;
        }
      }
      else if(droneNeed > 0)
      {
        if(minerals >= 50 && usedSupply < supplyLimit)
        {
          for(auto& base : baseManager.bases())
            if(builder.train(sc2::UNIT_TYPEID::ZERG_DRONE, &base, sc2::UNIT_TYPEID::ZERG_LARVA, id))
              return;
        }
      }
      else if(minerals >= 50 && usedSupply < supplyLimit && poolState.unitCount() > 0)
      {
        for(auto& base : baseManager.bases())
            if(builder.train(sc2::UNIT_TYPEID::ZERG_ZERGLING, &base, sc2::UNIT_TYPEID::ZERG_LARVA, id))
              return;
      }
    }

    void Brain_Macro::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

  }

}
