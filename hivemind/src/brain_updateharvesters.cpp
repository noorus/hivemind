#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
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

      lastSaturateTime_ = bot_->time();
    }

    AI::Goal::Status Brain_UpdateHarvesters::process()
    {
      auto time = bot_->time();

      const GameTime saturateInterval = 16;

      if(time >= lastSaturateTime_ + saturateInterval)
      {
        lastSaturateTime_ = time;

        for ( auto& base : bot_->bases().bases() )
        {
          saturate(base);
        }
      }

      return status_;
    }

    void Brain_UpdateHarvesters::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

    static bool isHarvestOrder(const sc2::UnitOrder& order)
    {
      return order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER ||
        order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER_DRONE ||
        order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER_PROBE ||
        order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER_SCV;
    }

    static bool isHarvestReturnOrder(const sc2::UnitOrder& order)
    {
      return order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN ||
        order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN_DRONE ||
        order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN_MULE ||
        order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN_PROBE ||
        order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN_SCV;
    }

    void Brain_UpdateHarvesters::saturate(Base& base)
    {
      auto action = &bot_->action();

      auto& refineries = base.refineries();

      for(int i = 0; i < refineries.size(); ++i)
      {
        auto& refinery = refineries[i];
        if(refinery.refinery_->build_progress < 1.0f)
        {
          continue;
        }
        if(refinery.refinery_->vespene_contents <= 0 || !refinery.refinery_->is_alive)
        {
          for(auto worker : refinery.workers_)
          {
            base.addWorker(worker);
            refinery.workers_.clear();
          }
          continue;
        }

        const int maxRefineryWorkers = 3;

        // Detect if a worker has deserted its vespene collecting task (e.g. due to human actions).
        for(auto it = refinery.workers_.begin(); it != refinery.workers_.end(); )
        {
          auto worker = *it;
          bool workerIsDistracted = false;
          if(worker->orders.empty())
          {
            workerIsDistracted = true;
          }
          else
          {
            auto& order = worker->orders.front();
            if(isHarvestOrder(order))
            {
              if(order.target_unit_tag != refinery.refinery_->tag)
              {
                workerIsDistracted = true;
              }
            }
            else if(!isHarvestReturnOrder(order))
            {
              workerIsDistracted = true;
            }
          }

          if(workerIsDistracted)
          {
            base.addWorker(worker);
            it = refinery.workers_.erase(it);
          }
          else
          {
            ++it;
          }
        }

        // Always assign maximum workers to collect vespene.
        for(size_t j = refinery.workers_.size(); j < maxRefineryWorkers; ++j)
        {
          auto worker = base.releaseWorker();

          if(worker)
          {
            action->UnitCommand(worker, sc2::ABILITY_ID::HARVEST_GATHER, refinery.refinery_);
            refinery.workers_.insert(worker);
          }
        }
      }

      // Assign idle workers to minerals.
      auto& workers = base.workers();
      for(auto worker : workers)
      {
        if(worker->orders.empty())
        {
          auto location = base.location();
          auto& minerals = location->minerals_;

          if(!minerals.empty())
          {
            auto mineral = *minerals.begin();
            action->UnitCommand(worker, sc2::ABILITY_ID::HARVEST_GATHER, mineral);
          }
        }
      }

    }
  }

}
