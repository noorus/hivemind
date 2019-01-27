#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "ai_goals.h"
#include "ai_agent.h"
#include "messaging.h"
#include "hive_vector2.h"
#include "resource_type_id.h"

namespace hivemind {

  class Bot;
  class BuildPlanner;
  
  struct BuildPlan
  {
    enum class State
    {
      NotDone,
      Done
    };

    virtual ~BuildPlan()
    {
    }

    virtual AllocatedResources remainingCost(BuildPlanner* planner) const = 0;
    virtual State updateProgress(BuildPlanner* planner) = 0;
    virtual void executePlan(BuildPlanner* planner, AllocatedResources allocatedResources) = 0;
    virtual std::string toString() const = 0;
    virtual pair<ResourceTypeID, int> getCount() const = 0;
  };

  struct UnitBuildPlan : public BuildPlan
  {
    sc2::UNIT_TYPEID unitType_;
    size_t count_;

    std::set<BuildProjectID> startedBuilds_;

    explicit UnitBuildPlan(sc2::UNIT_TYPEID unitType, size_t count = 1):
      unitType_(unitType),
      count_(count)
    {
    }

    AllocatedResources remainingCost(BuildPlanner* planner) const override
    {
      size_t remaining = count_ - startedBuilds_.size();
      return Builder::getCost(unitType_) * (int)remaining;
    }

    virtual State updateProgress(BuildPlanner* planner) override;
    virtual void executePlan(BuildPlanner* planner, AllocatedResources allocatedResources) override;
    virtual std::string toString() const override;
    virtual pair<ResourceTypeID, int> getCount() const override;
  };

  struct ResearchBuildPlan : public BuildPlan
  {
    sc2::UPGRADE_ID upgradeType_;

    BuildProjectID startedBuild_;

    explicit ResearchBuildPlan(sc2::UPGRADE_ID upgradeType):
      upgradeType_(upgradeType),
      startedBuild_(-1)
    {
    }

    AllocatedResources remainingCost(BuildPlanner* planner) const override;

    virtual State updateProgress(BuildPlanner* planner) override;
    virtual void executePlan(BuildPlanner* planner, AllocatedResources allocatedResources) override;
    virtual std::string toString() const override;
    virtual pair<ResourceTypeID, int> getCount() const override;
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

    std::unique_ptr<UnitBuildPlan> makeTechPlan(Builder& builder, sc2::UnitTypeID unitType) const;

    const std::vector<std::unique_ptr<BuildPlan>>& getPlans() const
    {
      return plans_;
    }

    UnitStats getStats(ResourceTypeID resourceType);

  private:
    Bot* bot_;

    std::vector<std::unique_ptr<BuildPlan>> plans_;
  };
}
