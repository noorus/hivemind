#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"

namespace hivemind {

  struct BuildPlan
  {
    enum class State
    {
      NotDone,
      Done
    };

    virtual int remainingCost() const = 0;
    virtual State updateProgress(BuildPlanner* planner) = 0;
    virtual void executePlan(BuildPlanner* planner) = 0;
    virtual std::string toString() const = 0;
  };

  struct UnitBuildPlan : public BuildPlan
  {
    sc2::UNIT_TYPEID unitType_;
    int count_;

    std::set<BuildProjectID> startedBuilds_;

    explicit UnitBuildPlan(sc2::UNIT_TYPEID unitType, int count):
      unitType_(unitType),
      count_(count)
    {
    }

    int remainingCost() const override
    {
      return 0;
    }

    virtual State updateProgress(BuildPlanner* planner) override;
    virtual void executePlan(BuildPlanner* planner) override;
    virtual std::string toString() const override;
  };

  struct StructureBuildPlan : public BuildPlan
  {
    sc2::UNIT_TYPEID building_;

    BuildProjectID startedBuild_;

    explicit StructureBuildPlan(sc2::UNIT_TYPEID building):
      building_(building),
      startedBuild_(-1)
    {
    }

    virtual State updateProgress(BuildPlanner* planner) override;
    virtual void executePlan(BuildPlanner* planner) override;
    virtual std::string toString() const override;

    int remainingCost() const override
    {
      return 0;
    }
  };

  class BuildPlanner
  {
  public:
    explicit BuildPlanner(Bot* bot) :
      bot_(bot)
    {
    }

    void updateProgress();

    void executePlans();

    void addPlan(std::unique_ptr<BuildPlan> plan)
    {
      bot_->console().printf("Added plan %s", plan->toString().c_str());

      plans_.push_back(std::move(plan));
    }

    Bot* getBot() const
    {
      return bot_;
    }

  private:
    Bot* bot_;

    std::vector<std::unique_ptr<BuildPlan>> plans_;
  };

  namespace Goals {

    Brain_Macro::Brain_Macro( AI::Agent * agent ):
      AI::CompositeGoal( agent ),
      planner_(new BuildPlanner(bot_))
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
      auto& baseManager = bot_->bases();

      if(baseManager.bases().empty())
      {
        return;
      }

      planner_->updateProgress();

      int minerals = bot_->Observation()->GetMinerals();
      int vespene = bot_->Observation()->GetVespene();
      int supplyLimit = bot_->Observation()->GetFoodCap();
      int usedSupply = bot_->Observation()->GetFoodUsed();

      auto& builder = bot_->builder();

      auto allocatedResources = builder.getAllocatedResources();
      minerals -= allocatedResources.minerals;
      vespene -= allocatedResources.vespene;
      usedSupply += allocatedResources.food;

      auto& poolState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
      auto& overlordState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_OVERLORD);
      auto& extractorState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_EXTRACTOR);
      auto& hatcheryState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_HATCHERY);
      auto& lairState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_LAIR);
      auto& hiveState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_HIVE);
      auto& droneState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_DRONE);
      auto& queenState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_QUEEN);
      auto& evolutionChamberState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER);

      int futureSupplyLimit = std::min(200, supplyLimit + overlordState.inProgressCount() * 8);
      int overlordNeed = futureSupplyLimit < 200 && (usedSupply >= futureSupplyLimit - 1 ? true : false);

      int hatcheryCount = hatcheryState.unitCount() + lairState.unitCount() + hiveState.unitCount();
      int hatcheryFutureCount = hatcheryState.futureCount() + lairState.unitCount() + hiveState.unitCount();

      int scoutDroneCount = 1;
      int futureDroneCount = droneState.futureCount();
      int droneNeed = futureDroneCount < hatcheryCount * (16 + 2 * 3) + scoutDroneCount && futureDroneCount <= 75;

      int futureExtractorCount = extractorState.futureCount();
      int extractorNeed = futureExtractorCount < hatcheryCount && (futureDroneCount > hatcheryCount * 16 + futureExtractorCount * 3 + 1 + scoutDroneCount || minerals > 400);

      int poolNeed = poolState.futureCount() == 0 && futureDroneCount >= 16 + 1 + scoutDroneCount;

      int queenNeed = poolState.unitCount() > 0 && queenState.futureCount() < hatcheryCount && queenState.inProgressCount() < hatcheryCount;

      int hatcheryNeed = droneState.futureCount() >= 17 * hatcheryFutureCount;

      auto lingSpeed = builder.getUpgradeStatus(sc2::UPGRADE_ID::ZERGLINGMOVEMENTSPEED);
      int lingSpeedNeed = poolState.unitCount() > 0 && lingSpeed == UpgradeStatus::NotStarted;

      int evolutionChamberNeed = evolutionChamberState.futureCount() == 0 && poolState.unitCount() > 0 && extractorState.unitCount() > 0 && queenState.futureCount() > 0;

      auto meleeAttack1 = builder.getUpgradeStatus(sc2::UPGRADE_ID::ZERGMELEEWEAPONSLEVEL1);
      int meleeAttack1Need = evolutionChamberState.unitCount() > 0 && meleeAttack1 == UpgradeStatus::NotStarted;

      int lairNeed = lairState.futureCount() == 0 && poolState.unitCount() > 0;

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

      /*
      if(lairNeed > 0)
      {
        if(minerals >= 150 && vespene >= 100)
        {
          for(auto& base : baseManager.bases())
            if(builder.train(sc2::UNIT_TYPEID::ZERG_LAIR, &base, sc2::UNIT_TYPEID::ZERG_HATCHERY, id))
              return;
        }
      }
      */

      /*
      if(poolNeed > 0)
      {
        if(minerals >= 200)
        {
          for(auto& base : baseManager.bases())
            if(builder.build(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, &base, BuildPlacement_Generic, id))
              return;
        }
      }
      */

      if(lingSpeedNeed > 0)
      {
        if(minerals >= 100 && vespene > 100)
        {
          for(auto& base : baseManager.bases())
            if(builder.research(sc2::UPGRADE_ID::ZERGLINGMOVEMENTSPEED, &base, sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, id))
              return;
        }
      }

      if(evolutionChamberNeed > 0)
      {
        if(minerals >= 75)
        {
          for(auto& base : baseManager.bases())
            if(builder.build(sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER, &base, BuildPlacement_Generic, id))
              return;
        }
      }

      if(meleeAttack1Need > 0)
      {
        if(minerals >= 100 && vespene > 100)
        {
          for(auto& base : baseManager.bases())
            if(builder.research(sc2::UPGRADE_ID::ZERGMELEEWEAPONSLEVEL1, &base, sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER, id))
              return;
        }
      }

      if(overlordNeed > 0)
      {
        if(minerals >= 100)
        {
          for(auto& base : baseManager.bases())
            if(builder.train(sc2::UNIT_TYPEID::ZERG_OVERLORD, &base, sc2::UNIT_TYPEID::ZERG_LARVA, id))
              return;
        }
      }

      if(queenNeed > 0)
      {
        if(minerals >= 150 && usedSupply + 1 < supplyLimit)
        {
          for(auto& base : baseManager.bases())
          {
            if(builder.train(sc2::UNIT_TYPEID::ZERG_QUEEN, &base, sc2::UNIT_TYPEID::ZERG_HATCHERY, id))
              return;
            if(builder.train(sc2::UNIT_TYPEID::ZERG_QUEEN, &base, sc2::UNIT_TYPEID::ZERG_LAIR, id))
              return;
            if(builder.train(sc2::UNIT_TYPEID::ZERG_QUEEN, &base, sc2::UNIT_TYPEID::ZERG_HIVE, id))
              return;
          }
        }
      }

      if(extractorNeed > 0)
      {
        if(minerals >= 50)
        {
          for(auto& base : baseManager.bases())
            if(builder.build(sc2::UNIT_TYPEID::ZERG_EXTRACTOR, &base, BuildPlacement_Extractor, id))
              return;
        }
      }

      {
        // Build units from larvae according to the plan.
        planner_->executePlans();
      }
    }

    void Brain_Macro::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }
  }

  void BuildPlanner::updateProgress()
  {
    erase_if(plans_,
      [this](auto& plan)
      {
        bool done = plan->updateProgress(this) == BuildPlan::State::Done;

        if(done)
        {
          bot_->console().printf("Finished plan %s", plan->toString().c_str());
        }

        return done;
      });
  }

  static bool haveTechToTrain(Builder& builder, sc2::UnitTypeID unitType)
  {
    std::vector<sc2::UnitTypeID> techChain;
    Database::techTree().findTechChain(unitType, techChain);

    for(auto& buildingType : techChain)
    {
      if(!utils::isStructure(buildingType))
        continue;

      auto& state = builder.getUnitStats(buildingType);
      if(state.unitCount() < 1)
      {
        return false;
      }
    }

    return true;
  }

  static std::unique_ptr<StructureBuildPlan> makeTechPlan(Builder& builder, sc2::UnitTypeID unitType)
  {
    std::vector<sc2::UnitTypeID> techChain;
    Database::techTree().findTechChain(unitType, techChain);

    for(auto& buildingType : techChain)
    {
      if(!utils::isStructure(buildingType))
        continue;

      auto& state = builder.getUnitStats(buildingType);
      if(state.unitCount() < 1)
      {

        return std::make_unique<StructureBuildPlan>(buildingType);
      }
    }

    return nullptr;
  }

  void BuildPlanner::executePlans()
  {
      int minerals = bot_->Observation()->GetMinerals();
      int vespene = bot_->Observation()->GetVespene();
      int supplyLimit = bot_->Observation()->GetFoodCap();
      int usedSupply = bot_->Observation()->GetFoodUsed();

      auto& baseManager = bot_->bases();
      auto& builder = bot_->builder();

      auto allocatedResources = builder.getAllocatedResources();
      minerals -= allocatedResources.minerals;
      vespene -= allocatedResources.vespene;
      usedSupply += allocatedResources.food;
#if 0
      auto& droneState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_DRONE);
      auto& hatcheryState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_HATCHERY);
      auto& lairState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_LAIR);
      auto& hiveState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_HIVE);
      auto& poolState = builder.getUnitStats(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);

      int hatcheryCount = hatcheryState.unitCount() + lairState.unitCount() + hiveState.unitCount();
      int hatcheryFutureCount = hatcheryState.futureCount() + lairState.unitCount() + hiveState.unitCount();

      int scoutDroneCount = 1;
      int futureDroneCount = droneState.futureCount();
      int droneNeed = futureDroneCount < hatcheryCount * (16 + 2 * 3) + scoutDroneCount && futureDroneCount <= 75;

      BuildProjectID id;
      if(droneNeed > 0)
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
#endif

      if(plans_.size() < 2 || plans_.size() < 10 && minerals > 400)
      {
        sc2::UNIT_TYPEID unitTypes[] =
        {
          sc2::UNIT_TYPEID::ZERG_DRONE,
          sc2::UNIT_TYPEID::ZERG_ZERGLING,
          sc2::UNIT_TYPEID::ZERG_ROACH,
        };
        const int N = sizeof(unitTypes) / sizeof(*unitTypes);

        static int choice = N - 1;
        choice = (choice + 1) % N;

        auto unitType = unitTypes[choice];
        int unitCount = 4;

        if(haveTechToTrain(builder, unitType))
        {
          auto plan = std::make_unique<UnitBuildPlan>(unitType, unitCount);
          if(plan)
          {
            addPlan(std::move(plan));
          }
        }
        else
        {
          auto plan = makeTechPlan(builder, unitType);
          if(plan)
          {
            addPlan(std::move(plan));
          }
        }
      }

      for(auto& plan : plans_)
      {
        plan->executePlan(this);
      }
  }

  void UnitBuildPlan::executePlan(BuildPlanner* planner)
  {
    Bot* bot = planner->getBot();

    auto& baseManager = bot->bases();
    auto& builder = bot->builder();

    if(startedBuilds_.size() < count_)
    {
      if(!builder.haveResourcesToMake(unitType_))
      {
        return;
      }

      BuildProjectID id;

      for(auto& base : baseManager.bases())
      {
        if(builder.train(unitType_, &base, sc2::UNIT_TYPEID::ZERG_LARVA, id))
        {
          startedBuilds_.insert(id);
          break;
        }
      }
    }
  }

  std::string UnitBuildPlan::toString() const
  {
    return "train " + std::to_string(count_) + " " + sc2::UnitTypeToName(unitType_);
  }

  StructureBuildPlan::State StructureBuildPlan::updateProgress(BuildPlanner* planner)
  {
    Bot* bot = planner->getBot();
    auto& builder = bot->builder();

    if(startedBuild_ == -1)
      return State::NotDone;

    if(!builder.isFinished(startedBuild_))
      return State::NotDone;

    return State::Done;
  }

  void StructureBuildPlan::executePlan(BuildPlanner* planner)
  {
    Bot* bot = planner->getBot();

    auto& baseManager = bot->bases();
    auto& builder = bot->builder();

    if(startedBuild_ == -1)
    {
      if(!builder.haveResourcesToMake(building_))
      {
        return;
      }

      BuildProjectID id;

      for(auto& base : baseManager.bases())
      {
        if(builder.build(building_, &base, BuildPlacement_Generic, id))
        {
          startedBuild_ = id;
          break;
        }
      }
    }
  }

  std::string StructureBuildPlan::toString() const
  {
    return std::string("build ") + sc2::UnitTypeToName(building_);
  }

  UnitBuildPlan::State UnitBuildPlan::updateProgress(BuildPlanner* planner)
  {
    Bot* bot = planner->getBot();
    auto& builder = bot->builder();

    if(startedBuilds_.size() < count_)
    {
      return State::NotDone;
    }

    for(auto& id : startedBuilds_)
    {
      if(!builder.isFinished(id))
        return State::NotDone;
    }

    return State::Done;
  }
}
