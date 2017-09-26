#pragma once
#include "sc2_forward.h"
#include "hive_math.h"

namespace hivemind {

  namespace utils {

    static std::random_device g_randomDevice;
    static std::mt19937 g_random( g_randomDevice() );

    constexpr GameTime timeToTicks( const uint16_t minutes, const uint16_t seconds )
    {
      const float speed_multiplier = 1.4f; // faster
      const GameTime ticks_per_second = 16; // 32 updates per second / 2 updates per tick
      return (GameTime)( ( ( minutes * 60 ) + seconds ) * ticks_per_second * speed_multiplier );
    }

    inline const RealTime ticksToTime( const GameTime ticks )
    {
      const float speed_multiplier = 1.4f; // faster
      const float ticks_per_second = 16.0f; // 32 updates per second / 2 updates per tick
      return (RealTime)math::round( ( (float)ticks / speed_multiplier ) / ticks_per_second );
    }

    inline int randomBetween( int imin, int imax )
    {
      std::uniform_int_distribution<> distrib( imin, imax );
      return distrib( g_random );
    }

    inline const bool pathable( const GameInfo& info, size_t x, size_t y )
    {
      uint8_t encoded = info.pathing_grid.data[x + ( info.height - 1 - y ) * info.width];
      bool decoded = ( encoded == 255 ? false : true );
      return decoded;
    }

    inline const bool placement( const GameInfo& info, size_t x, size_t y )
    {
      uint8_t encoded = info.placement_grid.data[x + ( info.height - 1 - y ) * info.width];
      bool decoded = ( encoded == 255 ? true : false );
      return decoded;
    }

    inline const Real terrainHeight( const GameInfo& info, size_t x, size_t y )
    {
      uint8_t encoded = info.terrain_height.data[x + ( info.height - 1 - y ) * info.width];
      Real decoded = ( -100.0f + 200.0f * Real( encoded ) / 255.0f );
      return decoded;
    }

    inline const Vector2 calculateCenter( const UnitVector& units )
    {
      Vector2 total( 0.0f, 0.0f );
      if ( units.empty() )
        return total;

      for ( auto unit : units )
      {
        total.x += unit->pos.x;
        total.y += unit->pos.y;
      }

      return ( total / (Real)units.size() );
    }

    inline const bool isMine( const Unit& unit ) { return ( unit.alliance == Unit::Alliance::Self ); }
    inline const bool isMine( const Unit* unit ) { return ( unit->alliance == Unit::Alliance::Self ); }

    inline const bool isEnemy( const Unit& unit ) { return ( unit.alliance == Unit::Alliance::Enemy ); }
    inline const bool isEnemy( const Unit* unit ) { return ( unit->alliance == Unit::Alliance::Enemy ); }

    inline const bool isNeutral( const Unit& unit ) { return ( unit.alliance == Unit::Alliance::Neutral ); }
    inline const bool isNeutral( const Unit* unit ) { return ( unit->alliance == Unit::Alliance::Neutral ); }

    struct isFlying {
      inline bool operator()( const Unit& unit ) {
        return unit.is_flying;
      }
    };

    inline const bool isMainStructure( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::ZERG_HATCHERY:
        case sc2::UNIT_TYPEID::ZERG_LAIR:
        case sc2::UNIT_TYPEID::ZERG_HIVE:
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS:
          return true;
        default:
          return false;
      }
    }

    inline const bool isMainStructure( const Unit& unit ) { return isMainStructure( unit.unit_type ); }
    inline const bool isMainStructure( const Unit* unit ) { return isMainStructure( unit->unit_type ); }

    struct isMainStructure {
      inline bool operator()( const Unit& unit ) {
        return hivemind::utils::isMainStructure( unit.unit_type );
      }
    };

    inline const bool isRefinery( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::TERRAN_REFINERY:
        case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR:
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR:
          return true;
        default:
          return false;
      }
    }

    inline const bool isRefinery( const Unit& unit ) { return isRefinery( unit.unit_type ); }
    inline const bool isRefinery( const Unit* unit ) { return isRefinery( unit->unit_type ); }

    inline const bool isMineral( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD:
        case sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD750:
        case sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD:
        case sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750:
          return true;
        default:
          return false;
      }
    }

    inline const bool isMineral( const Unit& unit ) { return isMineral( unit.unit_type ); }
    inline const bool isMineral( const Unit* unit ) { return isMineral( unit->unit_type ); }

    inline const bool isGeyser( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::NEUTRAL_VESPENEGEYSER:
        case sc2::UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER:
        case sc2::UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER:
          return true;
        default:
          return false;
      }
    }

    inline const bool isGeyser( const Unit& unit ) { return isGeyser( unit.unit_type ); }
    inline const bool isGeyser( const Unit* unit ) { return isGeyser( unit->unit_type ); }

    inline const bool isWorker( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::TERRAN_SCV:
        case sc2::UNIT_TYPEID::PROTOSS_PROBE:
        case sc2::UNIT_TYPEID::ZERG_DRONE:
        case sc2::UNIT_TYPEID::ZERG_DRONEBURROWED:
          return true;
        default:
          return false;
      }
    }

    inline const bool isWorker( const Unit& unit ) { return isWorker( unit.unit_type ); }
    inline const bool isWorker( const Unit* unit ) { return isWorker( unit->unit_type ); }

    inline const bool isSupplyProvider( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::ZERG_OVERLORD:
        case sc2::UNIT_TYPEID::PROTOSS_PYLON:
        case sc2::UNIT_TYPEID::PROTOSS_PYLONOVERCHARGED:
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
          return true;
        default:
          return false;
      }
    }

    inline const bool isSupplyProvider( const Unit& unit ) { return isSupplyProvider( unit.unit_type ); }
    inline const bool isSupplyProvider( const Unit* unit ) { return isSupplyProvider( unit->unit_type ); }

    inline const bool isBuilding( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        // Zerg
        case sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER:
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR:
        case sc2::UNIT_TYPEID::ZERG_GREATERSPIRE:
        case sc2::UNIT_TYPEID::ZERG_HATCHERY:
        case sc2::UNIT_TYPEID::ZERG_HIVE:
        case sc2::UNIT_TYPEID::ZERG_HYDRALISKDEN:
        case sc2::UNIT_TYPEID::ZERG_INFESTATIONPIT:
        case sc2::UNIT_TYPEID::ZERG_LAIR:
        case sc2::UNIT_TYPEID::ZERG_NYDUSNETWORK:
        case sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL:
        case sc2::UNIT_TYPEID::ZERG_SPINECRAWLER:
        case sc2::UNIT_TYPEID::ZERG_SPINECRAWLERUPROOTED:
        case sc2::UNIT_TYPEID::ZERG_SPIRE:
        case sc2::UNIT_TYPEID::ZERG_SPORECRAWLER:
        case sc2::UNIT_TYPEID::ZERG_SPORECRAWLERUPROOTED:
        case sc2::UNIT_TYPEID::ZERG_ULTRALISKCAVERN:
        // Terran
        case sc2::UNIT_TYPEID::TERRAN_ARMORY:
        case sc2::UNIT_TYPEID::TERRAN_BARRACKS:
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSFLYING:
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
        case sc2::UNIT_TYPEID::TERRAN_BUNKER:
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
        case sc2::UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
        case sc2::UNIT_TYPEID::TERRAN_FACTORY:
        case sc2::UNIT_TYPEID::TERRAN_FACTORYFLYING:
        case sc2::UNIT_TYPEID::TERRAN_FACTORYREACTOR:
        case sc2::UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
        case sc2::UNIT_TYPEID::TERRAN_FUSIONCORE:
        case sc2::UNIT_TYPEID::TERRAN_GHOSTACADEMY:
        case sc2::UNIT_TYPEID::TERRAN_MISSILETURRET:
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
        case sc2::UNIT_TYPEID::TERRAN_REFINERY:
        case sc2::UNIT_TYPEID::TERRAN_SENSORTOWER:
        case sc2::UNIT_TYPEID::TERRAN_STARPORT:
        case sc2::UNIT_TYPEID::TERRAN_STARPORTFLYING:
        case sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR:
        case sc2::UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
        case sc2::UNIT_TYPEID::TERRAN_REACTOR:
        case sc2::UNIT_TYPEID::TERRAN_TECHLAB:
        // Protoss
        case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR:
        case sc2::UNIT_TYPEID::PROTOSS_CYBERNETICSCORE:
        case sc2::UNIT_TYPEID::PROTOSS_DARKSHRINE:
        case sc2::UNIT_TYPEID::PROTOSS_FLEETBEACON:
        case sc2::UNIT_TYPEID::PROTOSS_FORGE:
        case sc2::UNIT_TYPEID::PROTOSS_GATEWAY:
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS:
        case sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON:
        case sc2::UNIT_TYPEID::PROTOSS_PYLON:
        case sc2::UNIT_TYPEID::PROTOSS_PYLONOVERCHARGED:
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSBAY:
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY:
        case sc2::UNIT_TYPEID::PROTOSS_STARGATE:
        case sc2::UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE:
        case sc2::UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL:
        case sc2::UNIT_TYPEID::PROTOSS_WARPGATE:
          return true;
        default:
          return false;
      }
    }

    inline  const bool isBuilding( const Unit& unit ) { return isBuilding( unit.unit_type ); }
    inline  const bool isBuilding( const Unit* unit ) { return isBuilding( unit->unit_type ); }

    inline void hsl2rgb( uint16_t hue, uint8_t sat, uint8_t lum, uint8_t rgb[3] )
    {
      uint16_t r_temp, g_temp, b_temp;
      uint8_t hue_mod;
      uint8_t inverse_sat = ( sat ^ 255 );
      hue = hue % 768;
      hue_mod = hue % 256;
      if ( hue < 256 ) {
        r_temp = hue_mod ^ 255;
        g_temp = hue_mod;
        b_temp = 0;
      } else if ( hue < 512 ) {
        r_temp = 0;
        g_temp = hue_mod ^ 255;
        b_temp = hue_mod;
      } else if ( hue < 768 ) {
        r_temp = hue_mod;
        g_temp = 0;
        b_temp = hue_mod ^ 255;
      } else {
        r_temp = 0;
        g_temp = 0;
        b_temp = 0;
      }
      r_temp = ( ( r_temp * sat ) / 255 ) + inverse_sat;
      g_temp = ( ( g_temp * sat ) / 255 ) + inverse_sat;
      b_temp = ( ( b_temp * sat ) / 255 ) + inverse_sat;
      r_temp = ( r_temp * lum ) / 255;
      g_temp = ( g_temp * lum ) / 255;
      b_temp = ( b_temp * lum ) / 255;
      rgb[0] = (uint8_t)r_temp;
      rgb[1] = (uint8_t)g_temp;
      rgb[2] = (uint8_t)b_temp;
    }

  }

}