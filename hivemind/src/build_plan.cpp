#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"
#include "build_plan.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( build_plan_debug, "Whether to print debug information about plans", false);


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

  static bool haveTechToMake(Builder& builder, ResourceTypeID type)
  {
    std::vector<sc2::UnitTypeID> techChain;

    if(type.isUpgradeType())
    {
      Database::techTree().findTechChain(type.upgradeType_, techChain);
    }
    else if(type.isUnitType())
    {
      Database::techTree().findTechChain(type.unitType_, techChain);
    }
    else
    {
      return false;
    }

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
          if(g_CVar_build_plan_debug.as_i() > 0)
          {
            bot_->console().printf("Finished plan %s", plan->toString().c_str());
          }
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

  void BuildPlanner::addPlan(std::unique_ptr<BuildPlan> plan)
  {
    if(g_CVar_build_plan_debug.as_i() > 0)
    {
      bot_->console().printf("Added plan %s", plan->toString().c_str());
    }

    plans_.push_back(std::move(plan));
  }

  std::unique_ptr<BuildPlan> BuildPlanner::makeTechPlan(Builder& builder, sc2::UnitTypeID unitType) const
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
        if(auto oldPlan = dynamic_cast<const BuildPlan*>(x.get()))
        {
          if(oldPlan->type_ == ResourceTypeID(buildingType))
          {
            found = true;
            break;
          }
        }
      }
      if(found)
        continue;

      if(!haveTechToMake(builder, ResourceTypeID(buildingType)))
        continue;

      return std::make_unique<BuildPlan>(ResourceTypeID(buildingType));
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

  void BuildPlan::executePlan(BuildPlanner* planner, AllocatedResources allocatedResources)
  {
    Bot* bot = planner->getBot();

    auto& baseManager = bot->bases();
    auto& builder = bot->builder();

    if(startedBuilds_.size() < count_)
    {
      if(!builder.haveResourcesToMake(type_, allocatedResources))
      {
        allocatedResources.food = 0;
        if(builder.haveResourcesToMake(type_, allocatedResources))
        {
          if(g_CVar_build_plan_debug.as_i() > 0)
          {
            planner->getBot()->console().printf("Canceling plan %s, not enough supply", type_.name_);
          }
          count_ = startedBuilds_.size();
        } 
        return;
      }

      if(!haveTechToMake(builder, type_))
      {
        return;
      }

      BuildProjectID id;

      for(auto& base : baseManager.bases())
      {
        if(builder.make(type_, &base, id))
        {
          startedBuilds_.insert(id);
          break;
        }
      }
    }
  }

  std::string BuildPlan::toString() const
  {
    size_t total = count_;
    size_t started = startedBuilds_.size();
    size_t remaining = total - started;

    return "make " + std::to_string(total) + " " + type_.name_ + " (" + std::to_string(remaining) + " not started)";
  }

  pair<ResourceTypeID, int> BuildPlan::getCount() const
  {
    size_t total = count_;
    size_t started = startedBuilds_.size();
    size_t remaining = total - started;

    return { type_, (int)remaining };
  }

  AllocatedResources BuildPlan::remainingCost(BuildPlanner* planner) const
  {
    size_t remaining = count_ - startedBuilds_.size();
    return planner->getBot()->builder().getCost(type_) * (int)remaining;
  }

  BuildPlan::State BuildPlan::updateProgress(BuildPlanner* planner)
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
