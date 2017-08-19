#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
// #include "baselocations.h"

namespace hivemind {

  namespace Analysis {

    void Map_BuildBasics( const GameInfo& info, size_t& width_out, size_t& height_out, Array2<uint64_t>& flags_out, Array2<Real>& heightmap_out );

    // Label values: 0 = unwalkable, -1 = contour, > 0 = object number
    void Map_ProcessContours( Array2<uint64_t> flags_in, Array2<int>& labels_out, ComponentVector& components_out );

    void Map_ComponentPolygons( ComponentVector& components_in, PolygonComponentVector& polygons_out, bool simplify = true, double simplify_distance_cost = 0.5, double simplify_stop_cost = 1.4 );

    void Map_FindResourceClusters( const sc2::ObservationInterface& observation, vector<UnitVector>& clusters_out, size_t minClusterSize = 4, Real maxResourceDistance = 14.0f );

  }

}