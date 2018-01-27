#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "trainer.h"
#include "database.h"
#include "controllers.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( trainer_debug, "Whether to show and print debug information on the trainer subsystem. 0 = none, 1 = basics, 2 = verbose", 1 );

  Trainer::Trainer(Bot* bot, BuildProjectID& idPool, std::unordered_map<sc2::UNIT_TYPEID, UnitStats>& unitStats) :
      Subsystem(bot),
      idPool_(idPool),
      unitStats_(unitStats)
  {
  }

  void Trainer::gameBegin()
  {
    trainingProjects_.clear();
    bot_->messaging().listen(Listen_Global, this);
  }

  UnitRef Trainer::getTrainer(Base& base, UnitTypeID trainerType) const
  {
    if(trainerType == sc2::UNIT_TYPEID::ZERG_LARVA)
    {
      auto& larvae = base.larvae();
      if(larvae.empty())
      {
        return nullptr;
      }

      auto larva = *larvae.begin();
      base.removeLarva(larva);

      return larva;
    }
    else if(trainerType == sc2::UNIT_TYPEID::ZERG_HATCHERY)
    {
      for(auto building : base.depots())
      {
        if(building->unit_type != trainerType)
          continue;
        if(building->build_progress < 1.0f)
          continue;

        if(!building->orders.empty())
          continue;
        if(trainers_.count(building) > 0)
          continue;

        return building;
      }
    }
    return nullptr;
  }

  bool Trainer::train( UnitTypeID unitType, Base& base, UnitTypeID trainerType, BuildProjectID& idOut )
  {
    auto trainer = getTrainer(base, trainerType);

    if(!trainer)
    {
      return false;
    }

    Training build( idPool_++, unitType, trainerType, trainer );

    trainingProjects_.push_back( build );

    if ( g_CVar_trainer_debug.as_i() > 1 )
      bot_->console().printf( "Trainer: New TrainOp %d for %s", build.id, sc2::UnitTypeToName( unitType ) );

    trainers_.insert(trainer);

    unitStats_[unitType].inProgress.insert(build.id);

    idOut = build.id;
    return true;
  }

  void Trainer::remove( BuildProjectID id )
  {
    for(auto& training : trainingProjects_)
    {
      if(training.id == id)
      {
        training.cancel = true;
      }
    }
  }

  void Trainer::gameEnd()
  {
    bot_->messaging().remove( this );
  }

  void Trainer::onMessage( const Message& msg )
  {
    auto trainingComplete = [this](auto it)
    {
      auto training = *it;

      if ( g_CVar_trainer_debug.as_i() > 1 )
        bot_->console().printf( "Trainer: Removing TrainOp %d (%s)", training.id, training.completed ? "completed" : "canceled" );

      if ( training.completed )
        bot_->messaging().sendGlobal( M_Training_Finished, training.id );
      else
        bot_->messaging().sendGlobal( M_Training_Canceled, training.id );

      trainers_.erase(training.trainer);
      auto& stats = unitStats_[training.type];
      stats.inProgress.erase(training.id);
      trainingProjects_.erase(it);
    };

    if ( msg.code == M_Global_UnitCreated )
    {
      UnitRef unit = msg.unit();
      auto& stats = unitStats_[unit->unit_type];

      if ( !utils::isMine( unit ) )
        return;

      if(utils::isStructure(unit))
        return;

      stats.units.insert(unit);

      for(auto it = trainingProjects_.begin(); it != trainingProjects_.end(); ++it)
      {
        auto& training = *it;
        if(training.type == unit->unit_type)
        {
          if(training.trainer->orders.empty())
          {
            // Hatchery produced a queen.
            training.completed = true;

            trainingComplete(it);
            break;
          }
        }
      }
    }
    else if(msg.code == M_Global_UnitDestroyed)
    {
      UnitRef unit = msg.unit();
      auto& stats = unitStats_[unit->unit_type];

      if(!utils::isMine(unit))
      {
        return;
      }

      for(auto it = trainingProjects_.begin(); it != trainingProjects_.end(); ++it)
      {
        auto& training = *it;
        if(training.trainer == unit)
        {
          if(unit->health > 0.0f)
          {
            // Egg hatched.
            training.completed = true;
          }
          else
          {
            // Egg or hatchery got destroyed.
            training.cancel = true;
          }

          trainingComplete(it);
          break;
        }
      }

      stats.units.erase(unit);
    }
  }

  void Trainer::update( const GameTime time, const GameTime delta )
  {
    auto verbose = ( g_CVar_trainer_debug.as_i() > 1 );

    const GameTime cTrainRecheckInterval = 50;

    for(auto& training : trainingProjects_)
    {
      if(!training.moneyAllocated)
      {
        if(training.trainer && training.trainer->is_alive && !training.trainer->orders.empty())
        {
          training.moneyAllocated = true;
          bot_->messaging().sendGlobal( M_Training_Started, training.id );

          if ( verbose )
            bot_->console().printf( "TrainOp %d: training started with %x", training.id, training.trainer );
        }
        else if ( training.nextUpdateTime <= time )
        {
          training.nextUpdateTime = time + cTrainRecheckInterval;

          if(training.trainer && training.trainer->is_alive && training.trainer->orders.empty())
          {
            bot_->unitDebugMsgs_[training.trainer] = "Trainer, Op " + std::to_string(training.id);

            if ( verbose )
              bot_->console().printf( "TrainOp %d: Got trainer %x for %s", training.id, training.trainer, sc2::UnitTypeToName( training.type ) );

            Larva(training.trainer).morph(training.type);
            training.tries++;
          }
          else
          {
            // TODO: Get new trainer?
          }
        }
      }
    }
  }

  std::pair<int,int> Trainer::getAllocatedResources() const
  {
    int mineralSum = 0;
    int vespeneSum = 0;
    for(auto& training : trainingProjects_)
    {
      if(training.moneyAllocated)
      {
        continue;
      }

      const auto& data = Database::units().at(training.type);
      mineralSum += data.mineralCost;
      vespeneSum += data.vespeneCost;
    }
    return { mineralSum, vespeneSum };
  }

}
