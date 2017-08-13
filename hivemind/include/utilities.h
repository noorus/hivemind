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
      return (RealTime)Math::round( ( (float)ticks / speed_multiplier ) / ticks_per_second );
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

    inline const bool isMine( const Unit& unit )
    {
      return ( unit.alliance == Unit::Alliance::Self );
    }

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

    inline const bool isWorker( const Unit& unit )
    {
      return isWorker( unit.unit_type );
    }

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

    inline  const bool isSupplyProvider( const Unit& unit )
    {
      return isSupplyProvider( unit.unit_type );
    }

  }

}