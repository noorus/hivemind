#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"
#include "opening.h"
#include "build_plan.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( draw_macro, "Whether to draw debug information about macro plans", true);

  static void makeMorePlans(BuildPlanner* planner);

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

    static void draw(Bot* bot, const BuildPlanner* buildPlanner)
    {
      if(!g_CVar_draw_macro.as_b())
        return;

      auto& debug = bot->debug();

      Point2D screenPosition = Point2D(0.03f, 0.3f);
      const Point2D increment(0.0f, 0.011f);
      for(auto& plan : buildPlanner->getPlans())
      {
        std::string text = plan->toString();

        debug.drawText(text, screenPosition, sc2::Colors::Teal);
        screenPosition += increment;
      }
      screenPosition += increment;
    }

    void Brain_Macro::update()
    {
      draw(bot_, planner_.get());

      auto& baseManager = bot_->bases();

      if(baseManager.bases().empty())
      {
        return;
      }

      planner_->updateProgress();

      if(planner_->getPlans().size() < 10)
      {
        makeMorePlans(planner_.get());
      }

      planner_->executePlans();
    }

    void Brain_Macro::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }
  }

  static bool makeOpeningPlans(BuildPlanner* planner)
  {
    static auto opening = [&]() {
      auto opening = getRandomOpening();
      planner->getBot()->console().printf("Using opening: '%s'", opening.name);
      return opening;
    }();

    pair<ResourceTypeID, size_t> plan;

    if(opening.progress < opening.items.size())
    {
      plan = opening.items.at(opening.progress);
      ++opening.progress;
    }
    else
    {
      return false;
    }

    auto type = plan.first;
    auto count = plan.second;

    assert(count > 0);

    if(type.isUnitType())
    {
      auto plan = std::make_unique<UnitBuildPlan>(type.unitType_, count);
      if(plan)
      {
        planner->addPlan(std::move(plan));
      }
    }
    else
    {
      auto plan = std::make_unique<ResearchBuildPlan>(type.upgradeType_);
      if(plan)
      {
        planner->addPlan(std::move(plan));
      }
    }
    return true;
  }

  static void makeHeuristicPlans(BuildPlanner* planner)
  {
    Bot* bot_ = planner->getBot();

    int minerals = bot_->Observation()->GetMinerals();
    int vespene = bot_->Observation()->GetVespene();
    int supplyLimit = bot_->Observation()->GetFoodCap();
    int usedSupply = bot_->Observation()->GetFoodUsed();

    auto& builder = bot_->builder();

    auto allocatedResources = builder.getAllocatedResources();
    minerals -= allocatedResources.minerals;
    vespene -= allocatedResources.vespene;
    usedSupply += allocatedResources.food;

    auto& poolState = planner->getStats(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
    auto& overlordState = planner->getStats(sc2::UNIT_TYPEID::ZERG_OVERLORD);
    auto& extractorState = planner->getStats(sc2::UNIT_TYPEID::ZERG_EXTRACTOR);
    auto& hatcheryState = planner->getStats(sc2::UNIT_TYPEID::ZERG_HATCHERY);
    auto& lairState = planner->getStats(sc2::UNIT_TYPEID::ZERG_LAIR);
    auto& hiveState = planner->getStats(sc2::UNIT_TYPEID::ZERG_HIVE);
    auto& droneState = planner->getStats(sc2::UNIT_TYPEID::ZERG_DRONE);
    auto& queenState = planner->getStats(sc2::UNIT_TYPEID::ZERG_QUEEN);
    auto& evolutionChamberState = planner->getStats(sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER);
    auto& lingState = planner->getStats(sc2::UNIT_TYPEID::ZERG_ZERGLING);
    auto& roachWarrenState = planner->getStats(sc2::UNIT_TYPEID::ZERG_ROACHWARREN);

    int futureSupplyLimit = std::min(200, supplyLimit + overlordState.inProgressCount() * 8);
    int overlordNeed = futureSupplyLimit < 200 &&
      (
      (usedSupply >= futureSupplyLimit - 1) ||
        (usedSupply >= futureSupplyLimit - 2 && usedSupply > 20) ||
        (usedSupply >= futureSupplyLimit - 3 && usedSupply > 30) ||
        (usedSupply >= futureSupplyLimit - 6 && usedSupply > 60)
        );

    int freeSupply = supplyLimit - usedSupply;

    int hatcheryCount = hatcheryState.unitCount() + lairState.unitCount() + hiveState.unitCount();
    int hatcheryFutureCount = hatcheryState.futureCount() + lairState.unitCount() + hiveState.unitCount();

    int scoutDroneCount = 1;
    int futureDroneCount = droneState.futureCount();
    int droneNeed = futureDroneCount < hatcheryCount * (16 + 2 * 3) + scoutDroneCount && futureDroneCount <= 75 && freeSupply > 0;

    int futureExtractorCount = extractorState.futureCount();
    int extractorNeed = futureExtractorCount < hatcheryCount * 2
      && (droneState.unitCount() > hatcheryCount * 16 + extractorState.unitCount() * 3)
      && (futureDroneCount > hatcheryCount * 16 + (futureExtractorCount + 1) * 3);

    int poolNeed = poolState.futureCount() == 0 && futureDroneCount >= 16 + 1 + scoutDroneCount;

    int queenNeed = poolState.unitCount() > 0 && queenState.futureCount() < hatcheryCount && queenState.inProgressCount() < hatcheryCount;

    int hatcheryNeed = droneState.unitCount() > 18 * hatcheryFutureCount;

    auto lingSpeed = planner->getStats(sc2::UPGRADE_ID::ZERGLINGMOVEMENTSPEED);
    int lingSpeedNeed = poolState.unitCount() > 0 && lingSpeed.futureCount() == 0;

    int evolutionChamberNeed = evolutionChamberState.futureCount() == 0 && poolState.unitCount() > 0 && extractorState.unitCount() > 0 && lingState.unitCount() >= 6;

    auto meleeAttack1 = planner->getStats(sc2::UPGRADE_ID::ZERGMELEEWEAPONSLEVEL1);
    int meleeAttack1Need = evolutionChamberState.unitCount() > 0 && meleeAttack1.futureCount() == 0;

    int lairNeed = lairState.futureCount() == 0 && poolState.unitCount() > 0 && queenState.unitCount() > 0;
    int roachWarrenNeed = poolState.unitCount() > 0 && extractorState.futureCount() > 0 && roachWarrenState.futureCount() == 0;

    int zerlingNeed = poolState.unitCount() > 0 && freeSupply > 0;
    int roachNeed = roachWarrenState.unitCount() > 0 && freeSupply > 1;

    vector<pair<ResourceTypeID, int>> choices;

    if(hatcheryNeed > 0)
    {
      int odds = 10 * (minerals / 100 + 1);
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_HATCHERY, odds });
    }

    if(lairNeed > 0 && vespene >= 100)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_LAIR, 1 });

    if(poolNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, 20 });

    if(lingSpeedNeed > 0 && vespene > 100)
      choices.push_back({ sc2::UPGRADE_ID::ZERGLINGMOVEMENTSPEED, 2 });

    if(evolutionChamberNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER, 2 });

    if(meleeAttack1Need > 0 && vespene > 100)
      choices.push_back({ sc2::UPGRADE_ID::ZERGMELEEWEAPONSLEVEL1, 2 });

    if(overlordNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_OVERLORD, 1 });

    if(queenNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_QUEEN, 20 });

    if(extractorNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_EXTRACTOR, 1 });

    if(roachWarrenNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_ROACHWARREN, 2 });

    if(droneNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_DRONE, 3 });

    if(zerlingNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_ZERGLING, 5 });

    if(roachNeed > 0)
      choices.push_back({ sc2::UNIT_TYPEID::ZERG_ROACH, 5 });

    choices.push_back({ sc2::UNIT_TYPEID::INVALID, 10 });

    auto chooseRandomly = [](auto& choices) -> size_t
    {
      int sum = 0;
      for(auto&& x : choices)
        sum += x.second;

      int r = utils::randomBetween(0, sum);
      int a = 0;
      for(size_t i = 0, e = choices.size(); i != e; ++i)
      {
        a += choices[i].second;

        if(a >= r)
          return i;
      }
      return 0;
    };

    size_t choice = chooseRandomly(choices);

    auto type = choices.at(choice).first;
    auto count = 1;

    if(type.isUnitType())
    {
      auto plan = std::make_unique<UnitBuildPlan>(type.unitType_, count);
      if(plan)
      {
        planner->addPlan(std::move(plan));
      }
    }
    else if(type.isUpgradeType())
    {
      auto plan = std::make_unique<ResearchBuildPlan>(type.upgradeType_);
      if(plan)
      {
        planner->addPlan(std::move(plan));
      }
    }
    else
    {
      // Do nothing.
    }
  }

  static void makeMorePlans(BuildPlanner* planner)
  {
    if ( makeOpeningPlans( planner ) )
    {
      return;
    }

    return makeHeuristicPlans(planner);
  }
}
