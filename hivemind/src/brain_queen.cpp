#include "stdafx.h"
#include "brain.h"
#include "brain_queen.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"

namespace hivemind {

  namespace Goals {

    Brain_Queen::Brain_Queen( AI::Agent * agent ):
      AI::CompositeGoal( agent )
    {
      bot_->messaging().listen( Listen_Global, this );
    }

    Brain_Queen::~Brain_Queen()
    {
      bot_->messaging().remove(this);
    }

    void Brain_Queen::activate()
    {
      status_ = AI::Goal::Status_Active;

      nextUpdateTime_ = bot_->time();
    }

    void Brain_Queen::onMessage( const Message& msg )
    {
    }

    UnitRef Brain_Queen::acquireInjectorQueen(Base* base, UnitRef hatchery)
    {
      auto queens = base->queens();

      erase_if(queens, [](auto queen) { return queen->energy < 25; });

      auto queen = utils::findClosestUnit(queens, hatchery->pos);
      if(queen)
      {
        base->releaseQueen(queen);
      }
      return queen;
    }

    void Brain_Queen::processLarvaInjections()
    {
      for(auto& base : bot_->bases().bases())
      {
        for(auto depot : base.depots())
        {
          bool larvaInjectFound = false;
          for(auto& buff : depot->buffs)
          {
            if(buff == sc2::BUFF_ID::QUEENSPAWNLARVATIMER)
            {
              larvaInjectFound = true;
              break;
            }
          }

          auto it = std::find_if(injects_.begin(), injects_.end(), [depot](auto& inject) {return inject.hatchery == depot; });
          if(it == injects_.end() && !larvaInjectFound)
          {
            injects_.push_back({ &base, depot, nullptr });
          }
          else if(it != injects_.end() && larvaInjectFound)
          {
            auto queen = it->queen;
            if(queen && queen->is_alive)
            {
              it->base->addQueen(queen);
            }

            injects_.erase(it);
          }
        }
      }

      for(auto it = injects_.begin(); it != injects_.end(); )
      {
        if(!it->hatchery->is_alive)
        {
          if(it->queen && it->queen->is_alive)
          {
            it->base->addQueen(it->queen);
          }
          it = injects_.erase(it);
        }
        else
        {
          ++it;
        }
      }

      for(auto& inject : injects_)
      {
        if(inject.queen && !inject.queen->is_alive)
        {
          inject.queen = nullptr;
        }

        if(!inject.queen)
        {
          inject.queen = acquireInjectorQueen(inject.base, inject.hatchery);
        }

        if(!inject.queen)
        {
          continue;
        }

        if(inject.queen->orders.empty() || inject.queen->orders.front().ability_id != sc2::ABILITY_ID::EFFECT_INJECTLARVA)
        {
          bot_->action().UnitCommand(inject.queen, sc2::ABILITY_ID::EFFECT_INJECTLARVA, inject.hatchery);
        }
      }
    }

    void Brain_Queen::processCreepTumorBuilds()
    {
      for(auto& base : bot_->bases().bases())
      {
        for(auto queen : base.queens())
        {
          if(queen->energy < 50)
            continue;

          auto creep = bot_->map().creep( base.location()->position() );
          if ( !creep )
            continue;
          if ( creep->fronts.empty() )
            continue;

          // try three times to find a random creep front that is less than 30 units away,
          // and plant a creep tumor in the middle tile of that front
          Vector2 destPt;
          for ( size_t i = 0; i < 3; i++ )
          {
            // fronts cannot be empty, so no need to check that
            auto index = utils::randomBetween( 0, (int)creep->fronts.size() - 1 );
            auto& front = creep->fronts[index];
            index = math::floor( (Real)front.size() / 2.0f );
            destPt = Vector2( (Real)front[index].x, (Real)front[index].y );
            if ( destPt.distance( queen->pos ) < 30.0f )
              break;
          }

          bot_->action().UnitCommand( queen, sc2::ABILITY_ID::BUILD_CREEPTUMOR_QUEEN, destPt );
        }
      }
    }

    AI::Goal::Status Brain_Queen::process()
    {
      const GameTime cUpdateInterval = 16;

      if(nextUpdateTime_ >= bot_->time())
      {
        return status_;
      }

      nextUpdateTime_ = bot_->time() + cUpdateInterval;

      processLarvaInjections();
      processCreepTumorBuilds();

      return status_;
    }

    void Brain_Queen::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

  }

}
