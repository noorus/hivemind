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

    ResourceTypeID type_;
    size_t count_;

    std::set<BuildProjectID> startedBuilds_;

    explicit BuildPlan(ResourceTypeID type, size_t count = 1):
      type_(type),
      count_(count)
    {
    }

    ~BuildPlan() = default;
    AllocatedResources remainingCost(BuildPlanner* planner) const;
    State updateProgress(BuildPlanner* planner);
    void executePlan(BuildPlanner* planner, AllocatedResources allocatedResources);
    std::string toString() const;
    pair<ResourceTypeID, int> getCount() const;
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

    std::unique_ptr<BuildPlan> makeTechPlan(Builder& builder, sc2::UnitTypeID unitType) const;

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
