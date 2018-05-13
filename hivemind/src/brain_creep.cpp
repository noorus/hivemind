#include "stdafx.h"
#include "brain.h"
#include "brain_creep.h"
#include "ai_goals.h"
#include "bot.h"
#include "controllers.h"

namespace hivemind {

  namespace Goals {

    /* Brain: Spread creep goal */

    const GameTime cCreepCheckDelay = 50;
    const GameTime cTumorFullCreepTime = utils::timeToTicks( 0, (uint16_t)( ( ( 21.0f - 3.0f ) / 2.0f ) * 0.8332f * 1.5f ) ); // ((fullGrownFootprint - spawnFootprint) / 2) * creepPeriod * goodMeasure

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
        nextCreepTime_ = ( bot_->time() + cCreepCheckDelay );

        auto tumors = bot_->observation().GetUnits( []( const Unit& unit ) -> bool
        {
          return ( utils::isMine( unit ) && unit.is_alive && unit.unit_type == UNIT_TYPEID::ZERG_CREEPTUMORBURROWED && unit.orders.empty() );
        } );

        auto abils = bot_->query().GetAbilitiesForUnits( tumors, false );

        assert( abils.size() == tumors.size() );

        for ( size_t i = 0; i < tumors.size(); ++i )
        {
          auto tumor = tumors[i];
          auto& strucdata = bot_->my().structure( tumor );
          if ( strucdata.completed_ && bot_->time() >= ( strucdata.completed_ + cTumorFullCreepTime ) )
          {
            bool hasAbility = false;
            for ( auto& abil : abils[i].abilities )
              if ( abil.ability_id == sc2::ABILITY_ID::BUILD_CREEPTUMOR || abil.ability_id == sc2::ABILITY_ID::BUILD_CREEPTUMOR_TUMOR )
              {
                hasAbility = true;
                break;
              }

            if ( hasAbility )
            {
              auto tumorPt = Vector2( tumor->pos );
              auto creep = bot_->map().creep( Vector2( tumorPt ) );
              if ( !creep || creep->fronts.empty() )
              {
                bot_->console().printf( "CREEP: Want to creep via tumor %x, but no fronts found (?!)", id( tumor ) );
              }
              else
              {
                Vector2 destPt;
                for ( size_t i = 0; i < 3; i++ )
                {
                  // fronts cannot be empty, so no need to check that
                  auto index = utils::randomBetween( 0, (int)creep->fronts.size() - 1 );
                  auto& front = creep->fronts[index];
                  index = math::floor( (Real)front.size() / 2.0f );
                  auto diff = Vector2( (Real)front[index].x, (Real)front[index].y ) - tumorPt;
                  if ( diff.length() >= 9.5f )
                  {
                    diff.normalise();
                    diff *= 9.0f;
                  }
                  destPt = tumorPt + diff;
                  if ( bot_->query().Placement( sc2::ABILITY_ID::BUILD_CREEPTUMOR_TUMOR, destPt ) )
                    break;
                  else
                    bot_->console().printf( "CREEP: Want to creep via tumor %x, but can't find placement at %i,%i", id( tumor ), (int)destPt.x, (int)destPt.y );
                }
                bot_->console().printf( "CREEP: Creeping via tumor %x to %i,%i", id( tumor ), (int)destPt.x, (int)destPt.y );
                CreepTumor( tumor ).tumor( destPt );
              }
            }
          }
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
