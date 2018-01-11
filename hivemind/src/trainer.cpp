#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "Trainer.h"
#include "database.h"

namespace hivemind {

  Trainer::Trainer( Bot* bot ): Subsystem( bot ), idPool_( 0 )
  {
  }

  void Trainer::gameBegin()
  {
    trainings_.clear();
    idPool_ = 0;
    bot_->messaging().listen( Listen_Global, this );
  }

  bool Trainer::add( UnitTypeID unit, const Base& base, UnitTypeID trainer, AbilityID ability, TrainingProjectID& idOut )
  {
    auto& larvae = base.larvae();
    if(larvae.empty())
    {
      return false;
    }

    Training build( idPool_++, unit, trainer, ability );
    build.trainer = *larvae.begin();

    bot_->bases().removeLarva(build.trainer);

    trainings_.push_back( build );
    bot_->console().printf( "Trainer: New TrainOp %d for %s", build.id, sc2::UnitTypeToName( unit ) );

    idOut = build.id;
    return true;
  }

  void Trainer::remove( TrainingProjectID id )
  {
    for(auto& training : trainings_)
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
    if ( msg.code == M_Global_UnitCreated )
    {
      if ( !utils::isMine( msg.unit() ) )
        return;
      for ( auto& training : trainings_ )
      {
      }
    }
    else if(msg.code == M_Global_UnitDestroyed)
    {
      UnitRef unit = msg.unit();
      if(!utils::isMine(unit))
      {
        return;
      }

      for ( auto& build : trainings_ )
      {
        if(build.trainer == unit)
        {
          if(unit->health > 0.0f)
          {
            // Egg hatched.
            build.completed = true;
          }
          else
          {
            // Egg got destroyed.
            build.cancel = true;
          }
        }
      }
    }
  }

  void Trainer::update( const GameTime time, const GameTime delta )
  {
    const GameTime cTrainRecheckInterval = 50;

    auto it = trainings_.begin();
    while ( it != trainings_.end() )
    {
      auto& training = ( *it );

      if ( training.completed || training.cancel )
      {
        if ( training.completed )
          bot_->messaging().sendGlobal( M_Training_Finished, training.id );
        else
          bot_->messaging().sendGlobal( M_Training_Canceled, training.id );

        bot_->console().printf( "Trainer: Removing TrainOp %d (%s)", training.id, training.completed ? "completed" : "canceled" );

        it = trainings_.erase( it );
        continue;
      }

      ++it;

      if(!training.moneyAllocated)
      {
        if(training.trainer && training.trainer->is_alive && !training.trainer->orders.empty())
        {
          training.moneyAllocated = true;
          bot_->messaging().sendGlobal( M_Training_Started, training.id );
          bot_->console().printf( "TrainOp %d: training started with %x", training.id, training.trainer );
        }
        else if ( training.nextUpdateTime <= time )
        {
          training.nextUpdateTime = time + cTrainRecheckInterval;

          if(training.trainer && training.trainer->is_alive && training.trainer->orders.empty())
          {
            bot_->unitDebugMsgs_[training.trainer] = "Trainer, Op " + std::to_string( training.id );
            bot_->console().printf( "TrainOp %d: Got trainer %x for %s", training.id, training.trainer, sc2::UnitTypeToName(training.type) );
            bot_->action().UnitCommand( training.trainer, training.buildAbility );
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
    for(auto& training : trainings_)
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