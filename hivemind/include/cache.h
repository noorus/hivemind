#pragma once
#include "hive_types.h"
#include "sc2_forward.h"
#include "hive_array2.h"
#include "utilities.h"
#include "console.h"
#include "map.h"

namespace hivemind {

  HIVE_EXTERN_CONVAR( cache_path );

  class Cache {
  private:
    static Bot* bot_;
  public:
    static void setBot( Bot* bot );
    static bool hasMapCache( const MapData& map, const string& name );
    static bool mapReadIntArray2( const MapData& map, Array2<int>& data, const string& name );
    static void mapWriteIntArray2( const MapData& map, Array2<int>& data, const string& name );
    static bool mapReadUint64Array2( const MapData& map, Array2<uint64_t>& data, const string& name );
    static void mapWriteUint64Array2( const MapData& map, Array2<uint64_t>& data, const string& name );
    static bool mapReadRegionVector( const MapData& map, RegionVector& regions, const string& name );
    static void mapWriteRegionVector( const MapData& map, RegionVector& regions, const string& name );
    static bool mapReadChokeVector( const MapData& map, ChokeVector& chokes, RegionVector& regions, const string& name );
    static void mapWriteChokeVector( const MapData& map, ChokeVector& chokes, const string& name );
    static bool mapReadRegionGraph( const MapData& map, OptimalRegionGraph& graph, const string& name );
    static void mapWriteRegionGraph( const MapData& map, OptimalRegionGraph& graph, const string& name );
  };

}