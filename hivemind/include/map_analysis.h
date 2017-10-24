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

    inline Polygon util_clipperPathToPolygon( ClipperLib::Path& in )
    {
      Polygon out;
      for ( auto& pt : in )
      {
        Real x = (Real)pt.X / 1000.0f;
        Real y = (Real)pt.Y / 1000.0f;
        out.emplace_back( x, y );
      }
      return out;
    }

    inline ClipperLib::Path util_polyToClipperPath( Polygon& in )
    {
      ClipperLib::Path out;
      for ( auto& pt : in )
      {
        auto x = ( ClipperLib::cInt )( pt.x * 1000.0f );
        auto y = ( ClipperLib::cInt )( pt.y * 1000.0f );
        out.emplace_back( x, y );
      }
      return out;
    }

    inline ClipperLib::Path util_contourToClipperPath( Contour& in )
    {
      ClipperLib::Path out;
      for ( auto& pt : in )
      {
        auto x = ( ClipperLib::cInt )( pt.x * 1000 );
        auto y = ( ClipperLib::cInt )( pt.y * 1000 );
        out.emplace_back( x, y );
      }
      return out;
    }

    inline BoostPolygon util_clipperPathToBoostPolygon( ClipperLib::Path& in )
    {
      BoostPolygon out;
      for ( auto &pt : in )
      {
        auto x = (double)pt.X / 1000.0;
        auto y = (double)pt.Y / 1000.0;
        boost::geometry::append( out, BoostPoint( x, y ) );
      }
      return out;
    }

    inline Polygon util_boostPolygonToPolygon( BoostPolygon& in )
    {
      Polygon out;
      boost::geometry::for_each_point( in, [&]( BoostPoint const& pt )
      {
        out.emplace_back( (Real)pt.x(), (Real)pt.y() );
      } );
      return out;
    }

    void Map_BuildBasics( const GameInfo& info, size_t& width_out, size_t& height_out, Array2<uint64_t>& flags_out, Array2<Real>& heightmap_out );

    // Label values: 0 = unwalkable, -1 = contour, > 0 = object number
    void Map_ProcessContours( Array2<uint64_t>& flags_in, Array2<int>& labels_out, ComponentVector& components_out );

    void Map_ComponentPolygons( ComponentVector& components_in, PolygonComponentVector& polygons_out, bool simplify = true, double simplify_distance_cost = 0.5, double simplify_stop_cost = 1.4 );

    void Map_InvertPolygons( const PolygonComponentVector& walkable_in, PolygonComponentVector& obstacles_out, const Rect2& playableArea, const Vector2& dimensions );

    void Map_MakeVoronoi( const GameInfo & info, PolygonComponentVector & polygons_in, Array2<int>& labels_in, RegionGraph& graph, bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree );

    void Map_PruneVoronoi( RegionGraph& graph );

    void Map_DetectNodes( RegionGraph& graph, const PolygonComponentVector& polygons );

    void Map_SimplifyGraph( const RegionGraph& graph, RegionGraph& graphSimplified );

    void Map_MergeRegionNodes( RegionGraph& graph );

    void Map_GetChokepointSides( const RegionGraph& graph, const bgi::rtree<BoostSegmentI, bgi::quadratic<16>>& rtree, std::map<RegionNodeID, Chokepoint>& chokepointSides );

    void Map_FindResourceClusters( const sc2::ObservationInterface& observation, vector<UnitVector>& clusters_out, size_t minClusterSize = 4, Real maxResourceDistance = 32.0f );

    void Map_MarkBaseTiles( Array2<uint64_t>& flagsmap, const BaseLocationVector& locations );

  }

}