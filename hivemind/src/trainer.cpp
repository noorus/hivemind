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

  static bool isHatcheryType(sc2::UnitTypeID unitType)
  {
    return unitType == sc2::UNIT_TYPEID::ZERG_HATCHERY ||
      unitType == sc2::UNIT_TYPEID::ZERG_LAIR ||
      unitType == sc2::UNIT_TYPEID::ZERG_HIVE;
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
    else if(isHatcheryType(trainerType))
    {
      for(auto building : base.depots())
      {
        if(!building->unit_type == trainerType)
        {
          continue;
        }
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

  bool Trainer::train( UnitTypeID unitType, Base* base, UnitTypeID trainerType, BuildProjectID& idOut )
  {
    auto trainer = getTrainer(*base, trainerType);

    if(!trainer)
    {
      return false;
    }

    Training training( idPool_++, unitType, trainerType, trainer );

    trainingProjects_.push_back( training );

    if ( g_CVar_trainer_debug.as_i() > 1 )
      bot_->console().printf( "TrainOp %d for %s: created with %x", training.id, sc2::UnitTypeToName( training.type ), id( training.trainer ) );

    trainers_.insert(trainer);

    unitStats_[unitType].inProgress.insert(training.id);

    idOut = training.id;
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

  void Trainer::onTrainingComplete(const Training& training)
  {
    if ( g_CVar_trainer_debug.as_i() > 1 )
      bot_->console().printf( "TrainOp %d for %s: removed with %x (%s)", training.id, sc2::UnitTypeToName( training.type ), id( training.trainer ), training.completed ? "completed" : "canceled" );

    if ( training.completed )
      bot_->messaging().sendGlobal( M_Training_Finished, training.id );
    else
      bot_->messaging().sendGlobal( M_Training_Canceled, training.id );

    trainers_.erase(training.trainer);
    auto& stats = unitStats_[training.type];
    stats.inProgress.erase(training.id);
  }

  void Trainer::onMessage( const Message& msg )
  {
    if ( msg.code == M_Global_UnitCreated )
    {
      UnitRef unit = msg.unit();

      if ( !utils::isMine( unit ) )
        return;

      if(utils::isStructure(unit))
        return;

      auto& stats = unitStats_[unit->unit_type];
      stats.units.insert(unit);

      for(auto it = trainingProjects_.begin(); it != trainingProjects_.end(); ++it)
      {
        auto& training = *it;
        if(training.type == unit->unit_type)
        {
          if(training.trainer->orders.empty() && training.moneyAllocated)
          {
            // Hatchery produced a queen.
            training.completed = true;

            onTrainingComplete(*it);
            break;
          }
        }
      }
    }
    else if(msg.code == M_Global_UnitDestroyed)
    {
      UnitRef unit = msg.unit();

      if(!utils::isMine(unit))
      {
        return;
      }

      auto& stats = unitStats_[unit->unit_type];

      for(auto it = trainingProjects_.begin(); it != trainingProjects_.end(); ++it)
      {
        auto& training = *it;
        if(training.trainer == unit)
        {
          if(unit->health > 0.0f)
          {
            // Egg or cocoon hatched.
            training.completed = true;
          }
          else
          {
            // Egg, cocoon or hatchery got destroyed.
            training.cancel = true;
          }

          onTrainingComplete(*it);
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
      if(training.completed || training.cancel)
        continue;

      if(!training.moneyAllocated)
      {
        if(!training.trainer->orders.empty())
        {
          training.moneyAllocated = true;
          bot_->messaging().sendGlobal( M_Training_Started, training.id );

          if ( verbose )
            bot_->console().printf( "TrainOp %d for %s: started with %x", training.id, sc2::UnitTypeToName( training.type ), id( training.trainer ) );
        }
        else if ( training.nextUpdateTime <= time )
        {
          training.nextUpdateTime = time + cTrainRecheckInterval;

          if(training.trainer && training.trainer->is_alive && training.trainer->orders.empty())
          {
            bot_->unitDebugMsgs_[training.trainer] = "TrainOp " + std::to_string(training.id);

            if ( verbose )
              bot_->console().printf( "TrainOp %d for %s: starting with %x", training.id, sc2::UnitTypeToName( training.type ), id( training.trainer ) );

            Larva(training.trainer).morph(training.type);
            training.tries++;
          }
        }
      }
      else
      {
        if(training.trainer->orders.empty())
        {
          // Hatchery was upgraded to a lair.
          assert(training.trainer->unit_type != training.trainerType);

          auto& statsOld = unitStats_[training.trainerType];
          statsOld.units.erase(training.trainer);

          auto& statsNew = unitStats_[training.trainer->unit_type];
          statsNew.units.insert(training.trainer);

          training.completed = true;
          onTrainingComplete(training);
        }
      }
    }

    erase_if(trainingProjects_, [](const auto& training) {return training.completed || training.cancel; });
  }

  AllocatedResources Trainer::getAllocatedResources() const
  {
    int mineralSum = 0;
    int vespeneSum = 0;
    int foodSum = 0;
    for(auto& training : trainingProjects_)
    {
      if(training.moneyAllocated)
      {
        continue;
      }

      auto ability = Database::techTree().getBuildAbility( training.type, training.trainerType );

      mineralSum += ability.mineralCost;
      vespeneSum += ability.vespeneCost;
      foodSum += ability.supplyCost;
    }
    return { mineralSum, vespeneSum, foodSum };
  }

}
