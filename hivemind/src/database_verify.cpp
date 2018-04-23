#include "stdafx.h"
#include "bot.h"
#include "utilities.h"
#include "database.h"
#include "controllers.h"

namespace hivemind {

  /*void Bot::verifyData()
  {
    auto apiTypeData = observation_->GetUnitTypeData();

    vector<UnitTypeID> unitTypes = {
      sc2::UNIT_TYPEID::TERRAN_MARINE,
      sc2::UNIT_TYPEID::PROTOSS_ZEALOT,
      sc2::UNIT_TYPEID::ZERG_ZERGLING,
      sc2::UNIT_TYPEID::TERRAN_MARAUDER,
      sc2::UNIT_TYPEID::PROTOSS_STALKER,
      sc2::UNIT_TYPEID::ZERG_BANELING,
      sc2::UNIT_TYPEID::TERRAN_REAPER,
      sc2::UNIT_TYPEID::PROTOSS_ADEPT,
      sc2::UNIT_TYPEID::ZERG_ROACH,
      sc2::UNIT_TYPEID::PROTOSS_ARCHON,
      sc2::UNIT_TYPEID::ZERG_QUEEN,
      sc2::UNIT_TYPEID::PROTOSS_IMMORTAL
    };

    auto checki = []( int a, int b ) -> string
    {
      if ( a == b )
        return "ok";
      else
      {
        char asd[128];
        sprintf_s( asd, 128, "fail: %i (db) - %i (sc2)", a, b );
        return asd;
      }
    };

    auto checkf = []( Real a, Real b ) -> string
    {
      if ( math::abs( a - b ) < 0.0001f )
        return "ok";
      else
      {
        char asd[128];
        sprintf_s( asd, 128, "fail: %.4f (db) - %.4f (sc2)", a, b );
        return asd;
      }
    };

    for ( auto uid : unitTypes )
    {
      console_.printf( "Verifying: %s", UnitTypeToName( uid ) );
      auto sc2Data = apiTypeData[uid];
      auto dbData = Database::unit( uid );
      console_.printf( "- minerals: %s", checki( dbData.mineralCost, sc2Data.mineral_cost ).c_str() );
      console_.printf( "- vespene: %s", checki( dbData.vespeneCost, sc2Data.vespene_cost ).c_str() );
      console_.printf( "- speed: %s", checkf( dbData.speed, sc2Data.movement_speed ).c_str() );
      console_.printf( "- sight: %s", checkf( dbData.sight, sc2Data.sight_range ).c_str() );
      console_.printf( "- armor: %s", checkf( (float)dbData.lifeArmor, sc2Data.armor ).c_str() );
      console_.printf( "- weapon count: %s", checki( (int)dbData.weapons.size(), (int)sc2Data.weapons.size() ).c_str() );
      // TODO: Try to figure out the same weapon(s) for comparison, by melee flags or range or so
      if ( !dbData.weapons.empty() && !sc2Data.weapons.empty() )
      {
        if ( dbData.weapons.size() < sc2Data.weapons.size() )
          console_.printf( "- wtf?! db has less weapons than sc2 (%d vs %d) - not parsing", dbData.weapons.size(), sc2Data.weapons.size() );
        else
        {
          // auto dbWpn = Database::weapon( uid );
          // auto sc2Wpn = sc2Data.weapons.back();
          for ( auto& sc2Wpn : sc2Data.weapons )
          {
            const WeaponData* dbWpn = nullptr;
            for ( auto& dbWpnName : dbData.weapons )
            {
              if ( !dbWpnName.empty() )
              {
                auto& wpn = Database::weapons().at( dbWpnName );
                if ( !dbWpn || math::abs( sc2Wpn.range - wpn.range ) < math::abs( sc2Wpn.range - dbWpn->range ) )
                  dbWpn = &wpn;
              }
            }
            if ( !dbWpn )
            {
              console_.printf( "--- wtf?! no db counterpart for weapon entry" );
            }
            else
            {
              console_.printf( "--- damage: %s", checkf( dbWpn->calculateBasicDamage(), sc2Wpn.damage_ ).c_str() );
              console_.printf( "--- range: %s", checkf( dbWpn->range, sc2Wpn.range ).c_str() );
              console_.printf( "--- period: %s", checkf( dbWpn->period, sc2Wpn.speed ).c_str() );
              // Don't bother checking attribute bonuses, since the API has all zeroes for now
              /*for ( size_t i = 0; i < (size_t)Attribute::Invalid; i++ )
              {
              auto dbVal = dbWpn->calculateAttributeBonuses( (Attribute)i );
              auto sc2Val = 0.0f;
              for ( auto& v : sc2Wpn.damage_bonus )
              if ( v.attribute == (Attribute)i )
              sc2Val = v.bonus;
              console_.printf( "--- attribute bonus %d: %s", i, checkf( dbVal, sc2Val ).c_str() );
              }*
            }
          }
        }
      }
    }
  }*/

}