#include "stdafx.h"
#include "brain.h"
#include "brain_queen.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"
#include "controllers.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( queen_debug, "Whether to show and print debug information on queens. 0 = none, 1 = basics, 2 = verbose", 1 );

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

          auto it = std::find_if(injectTasks_.begin(), injectTasks_.end(), [depot](auto& inject) {return inject.hatchery == depot; });
          if(it == injectTasks_.end() && !larvaInjectFound)
          {
            injectTasks_.push_back({ &base, depot, nullptr });
          }
          else if(it != injectTasks_.end() && larvaInjectFound)
          {
            auto queen = it->queen;
            if(queen && queen->is_alive)
            {
              it->base->addQueen(queen);
            }

            injectTasks_.erase(it);
          }
        }
      }

      for(auto it = injectTasks_.begin(); it != injectTasks_.end(); )
      {
        if(!it->hatchery->is_alive)
        {
          if(it->queen && it->queen->is_alive)
          {
            it->base->addQueen(it->queen);
          }
          it = injectTasks_.erase(it);
        }
        else
        {
          ++it;
        }
      }

      for(auto& inject : injectTasks_)
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
          Queen( inject.queen ).inject( inject.hatchery );
        }
      }
    }

    UnitRef Brain_Queen::acquireTumorQueen(Base* base)
    {
      for(auto queen : base->queens())
      {
        if(queen->energy < 25)
          continue;

        if(!queen->orders.empty())
          continue;

        base->releaseQueen(queen);

        return queen;
      }
      return nullptr;
    }

    boost::optional<Vector2> Brain_Queen::chooseTumorLocation(Base* base, UnitRef queen) const
    {
      auto creep = bot_->map().creep( base->location()->position() );
      if ( !creep )
        return boost::none;
      if ( creep->fronts.empty() )
        return boost::none;

      // try three times to find a random creep front that is less than 30 units away,
      // and plant a creep tumor in the middle tile of that front
      for ( size_t i = 0; i < 3; i++ )
      {
        assert(!creep->fronts.empty());

        auto frontIndex = utils::randomBetween( 0, (int)creep->fronts.size() - 1 );
        auto& front = creep->fronts[frontIndex];

        auto middleIndex = math::floor( (Real)front.size() / 2.0f );
        auto destPt = Vector2( (Real)front[middleIndex].x, (Real)front[middleIndex].y );

        if ( destPt.distance( queen->pos ) < 30.0f )
          return destPt;
      }

      return boost::none;
    }

    void Brain_Queen::processCreepTumorBuilds()
    {
      for(auto it = tumorTasks_.begin(); it != tumorTasks_.end(); )
      {
        auto tumorTaskIsOver = [](const auto& task) -> bool
        {
          UnitRef queen = task.queen;
          if(!queen->is_alive)
            return true;
          if(queen->orders.empty())
            return true;
          if(queen->orders.back().ability_id != sc2::ABILITY_ID::BUILD_CREEPTUMOR_QUEEN)
            return true;
          return false;
        };

        if(tumorTaskIsOver(*it))
        {
          if(it->queen->is_alive)
          {
            it->base->addQueen(it->queen);

            bot_->console().printf("Brain_Queen: Returning tumor queen %x from (%d,%d)", id( it->queen ), (int)it->tumorLocation.x, (int)it->tumorLocation.y );
          }
          it = tumorTasks_.erase(it);
        }
        else
        {
          ++it;
        }
      }

      for(auto& base : bot_->bases().bases())
      {
        UnitRef queen = acquireTumorQueen(&base);
        if(!queen)
          continue;

        auto tumorLocation = chooseTumorLocation(&base, queen);
        if(tumorLocation)
        {
          tumorTasks_.push_back({ &base, queen, *tumorLocation });

          Queen( queen ).tumor( *tumorLocation );

          if(g_CVar_queen_debug.as_i() > 0)
          {
            bot_->console().printf("Brain_Queen: Planting tumor to (%d,%d) with queen %x", (int)tumorLocation->x, (int)tumorLocation->y, id( queen ));
          }
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
