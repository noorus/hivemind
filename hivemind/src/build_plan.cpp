#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"
#include "build_plan.h"

namespace hivemind {

  static std::vector<sc2::UnitTypeID> getReverseTechAliases(sc2::UnitTypeID unitType)
  {
    if(unitType == sc2::UNIT_TYPEID::ZERG_HATCHERY)
    {
      return { sc2::UNIT_TYPEID::ZERG_HATCHERY, sc2::UNIT_TYPEID::ZERG_LAIR, sc2::UNIT_TYPEID::ZERG_HIVE };
    }
    if(unitType == sc2::UNIT_TYPEID::ZERG_LAIR)
    {
      return  { sc2::UNIT_TYPEID::ZERG_LAIR, sc2::UNIT_TYPEID::ZERG_HIVE };
    }
    if(unitType == sc2::UNIT_TYPEID::ZERG_SPIRE)
    {
      return { sc2::UNIT_TYPEID::ZERG_SPIRE, sc2::UNIT_TYPEID::ZERG_GREATERSPIRE };
    }
    if(unitType == sc2::UNIT_TYPEID::ZERG_HYDRALISKDEN)
    {
      return { sc2::UNIT_TYPEID::ZERG_HYDRALISKDEN, sc2::UNIT_TYPEID::ZERG_LURKERDENMP };
    }
    return { unitType };
  }

  static bool haveTechToMake(Builder& builder, sc2::UnitTypeID unitType)
  {
    std::vector<sc2::UnitTypeID> techChain;
    Database::techTree().findTechChain(unitType, techChain);

    for(auto& buildingType : techChain)
    {
      if(!utils::isStructure(buildingType))
        continue;

      bool found = false;
      for(auto alias : getReverseTechAliases(buildingType))
      {
        auto& stats = builder.getUnitStats(alias);
        if(stats.unitCount() >= 1)
        {
          found = true;
          break;
        }
      }
      if(!found)
      {
        return false;
      }
    }

    return true;
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

  void BuildPlanner::executePlans()
  {
    AllocatedResources allocatedResources{ 0, 0, 0 };
    for(auto& plan : plans_)
    {
      plan->executePlan(this, allocatedResources);
      allocatedResources = allocatedResources + plan->remainingCost(this);
    }
  }

  std::unique_ptr<UnitBuildPlan> BuildPlanner::makeTechPlan(Builder& builder, sc2::UnitTypeID unitType) const
  {
    std::vector<sc2::UnitTypeID> techChain;
    Database::techTree().findTechChain(unitType, techChain);

    for(auto& buildingType : boost::adaptors::reverse(techChain))
    {
      if(!utils::isStructure(buildingType))
        continue;

      bool found = false;
      for(auto alias : getReverseTechAliases(buildingType))
      {
        auto& stats = builder.getUnitStats(alias);
        if(stats.futureCount() >= 1)
        {
          found = true;
          break;
        }
      }
      if(found)
        continue;

      for(auto& x : plans_)
      {
        if(auto oldPlan = dynamic_cast<const UnitBuildPlan*>(x.get()))
        {
          if(oldPlan->unitType_ == buildingType)
          {
            found = true;
            break;
          }
        }
      }
      if(found)
        continue;

      if(!haveTechToMake(builder, buildingType))
        continue;

      return std::make_unique<UnitBuildPlan>(buildingType);
    }

    return nullptr;
  }

  UnitStats BuildPlanner::getStats(ResourceTypeID resourceType)
  {
    UnitStats stats;
    if(resourceType.isUnitType())
      stats = bot_->builder().getUnitStats(resourceType.unitType_);
    else if(resourceType.isUpgradeType())
      stats = bot_->builder().getUpgradeStatus(resourceType.upgradeType_);

    for(auto&& p : this->plans_)
    {
      auto* plan = p.get();
      auto job = plan->getCount();
      if(job.first == resourceType)
        stats.inProgressCount_ += job.second;
    }

    return stats;
  }

  void UnitBuildPlan::executePlan(BuildPlanner* planner, AllocatedResources allocatedResources)
  {
    Bot* bot = planner->getBot();

    auto& baseManager = bot->bases();
    auto& builder = bot->builder();

    if(startedBuilds_.size() < count_)
    {
      if(!builder.haveResourcesToMake(unitType_, allocatedResources))
      {
        return;
      }

      if(!haveTechToMake(builder, unitType_))
      {
        return;
      }

      BuildProjectID id;

      for(auto& base : baseManager.bases())
      {
        if(builder.make(unitType_, &base, id))
        {
          startedBuilds_.insert(id);
          break;
        }
      }
    }
  }

  std::string UnitBuildPlan::toString() const
  {
    size_t total = count_;
    size_t started = startedBuilds_.size();
    size_t remaining = total - started;

    return "make " + std::to_string(total) + " " + sc2::UnitTypeToName(unitType_) + " (" + std::to_string(remaining) + " not started)";
  }

  pair<ResourceTypeID, int> UnitBuildPlan::getCount() const
  {
    size_t total = count_;
    size_t started = startedBuilds_.size();
    size_t remaining = total - started;

    return { unitType_, (int)remaining };
  }

  BuildPlan::State UnitBuildPlan::updateProgress(BuildPlanner* planner)
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
  
  void ResearchBuildPlan::executePlan(BuildPlanner* planner, AllocatedResources allocatedResources)
  {
    Bot* bot = planner->getBot();

    auto& baseManager = bot->bases();
    auto& builder = bot->builder();

    if(startedBuild_ == -1)
    {
      if(!builder.haveResourcesToMake(upgradeType_, allocatedResources))
      {
        return;
      }

      // TODO: Should check if the upgrade is possible to make.
      //if(!haveTechToMake(builder, upgradeType_))
      //{
      //  return;
      //}

      BuildProjectID id;

      auto researcherType = getResearcherType(upgradeType_);

      for(auto& base : baseManager.bases())
      {
        if(builder.research(upgradeType_, &base, researcherType, id))
        {
          startedBuild_ = id;
          break;
        }
      }
    }
  }

  std::string ResearchBuildPlan::toString() const
  {
    size_t started = startedBuild_ != -1;

    return string("research ") + sc2::UpgradeIDToName(upgradeType_) + (started ? " (started)" : "");
  }

  pair<ResourceTypeID, int> ResearchBuildPlan::getCount() const
  {
    int count = startedBuild_ == -1;
    return { upgradeType_, count };
  }


  BuildPlan::State ResearchBuildPlan::updateProgress(BuildPlanner* planner)
  {
    Bot* bot = planner->getBot();
    auto& builder = bot->builder();

    if(startedBuild_ == -1)
    {
      return State::NotDone;
    }

    if(!builder.isFinished(startedBuild_))
      return State::NotDone;

    return State::Done;
  }

  AllocatedResources ResearchBuildPlan::remainingCost(BuildPlanner* planner) const
  {
    bool started = startedBuild_ != -1;
    size_t remaining = 1 - (int)started;
    return planner->getBot()->builder().getCost(upgradeType_) * (int)remaining;
  }

}
