#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Spread creep goal */

    const GameTime cCreepCheckDelay = 250;

    Brain_SpreadCreep::Brain_SpreadCreep( AI::Agent * agent ):
    AI::CompositeGoal( agent )
    {
    }

    void Brain_SpreadCreep::activate()
    {
      status_ = AI::Goal::Status_Active;
      nextCreepTime_ = bot_->time();
    }

    AI::Goal::Status Brain_SpreadCreep::process()
    {
      if ( nextCreepTime_ <= bot_->time() )
      {
        nextCreepTime_ = bot_->time() + cCreepCheckDelay;
        for ( auto& base : bot_->bases().bases() )
        {
          auto type = sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL;
          BuildProjectID buildId;
          bot_->builder().add( type, base, sc2::ABILITY_ID::BUILD_SPAWNINGPOOL, buildId );
          /*auto creep = bot_->map().creep( base.location()->position() );
          if ( !creep )
            continue;

          auto queen = base.queen();
          if ( !queen )
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

          bot_->action().UnitCommand( queen, sc2::ABILITY_ID::BUILD_CREEPTUMOR_QUEEN, destPt );*/
        }
      }
      return status_;
    }

    void Brain_SpreadCreep::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

  }

}