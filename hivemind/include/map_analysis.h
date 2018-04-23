#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"
#include "regiongraph.h"
#include "hive_rect2.h"

namespace hivemind {

  namespace Analysis {

    struct ChokeSides {
      Vector2 side1;
      Vector2 side2;
      Region* region1;
      Region* region2;
      ChokeSides( Vector2 s1, Vector2 s2 ): side1( s1 ), side2( s2 ) {}
      inline const Vector2 middle() const
      {
        return ( side1 + ( ( side2 - side1 ) * 0.5f ) );
      }
    };

    using RegionChokesMap = std::map<RegionNodeID, ChokeSides>;

    void Map_BuildBasics( const sc2::ObservationInterface& observation, size_t& width_out, size_t& height_out, Array2<uint64_t>& flags_out, Array2<Real>& heightmap_out );

    // Label values: 0 = unwalkable, -1 = contour, > 0 = object number
    void Map_ProcessContours( Array2<uint64_t>& flags_in, Array2<int>& labels_out, ComponentVector& components_out );

    void Map_ComponentPolygons( ComponentVector& components_in, PolygonComponentVector& polygons_out );

    void Map_InvertPolygons( const PolygonComponentVector& walkable_in, PolygonComponentVector& obstacles_out, const Rect2& playableArea, const Vector2& dimensions );

    void Map_MakeVoronoi( const GameInfo & info, PolygonComponentVector & polygons_in, Array2<int>& labels_in, RegionGraph& graph, bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree );

    void Map_PruneVoronoi( RegionGraph& graph );

    void Map_DetectNodes( RegionGraph& graph );

    void Map_SimplifyGraph( const RegionGraph& graph, RegionGraph& graphSimplified );

    void Map_MergeRegionNodes( RegionGraph& graph );

    void Map_GetChokepointSides( const RegionGraph& graph, const bgi::rtree<BoostSegmentI, bgi::quadratic<16>>& rtree, RegionChokesMap& chokepointSides );

    //! in: polygons, chokesidesmap, flagsmap, width, height, graph
    //! out: regions, regionlabelmap
    void Map_MakeRegions( const PolygonComponentVector& polygons,
      const RegionChokesMap& chokepointSides, Array2<uint64_t>& flagsmap, size_t width, size_t height,
      RegionVector& regions, Array2<int>& regionLabelMap, const RegionGraph& graph );

    void Map_FindResourceClusters( const sc2::ObservationInterface& observation, vector<UnitVector>& clusters_out, size_t minClusterSize = 4, Real maxResourceDistance = 32.0f );

    void Map_CacheClosestRegions( RegionVector& regions, Array2<int>& regionMap, Array2<int>& closest_out );

    void Map_MarkBaseTiles( Array2<uint64_t>& flagsmap, const BaseLocationVector& locations );

    void Map_CalculateRegionHeights( Array2<uint64_t>& flagsmap, RegionVector& regions_out, Array2<int>& regionMap, Array2<Real>& heightmap );

    void Map_ConnectChokepoints( const RegionChokesMap& chokepointSides, RegionVector& regions, Array2<int>& closestRegionMap, const RegionGraph& graph, ChokeVector& chokes_out_for_real );

  }

}