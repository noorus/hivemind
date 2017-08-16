#include "stdafx.h"
#include "map.h"
#include "map_analysis.h"
#include "utilities.h"
#include "blob_algo.h"
#include "exception.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polyline_simplification_2/simplify.h>

namespace hivemind {

  namespace Analysis {

    typedef CGAL::Exact_predicates_inexact_constructions_kernel CGAL_Kernel;
    typedef CGAL_Kernel::Point_2 CGAL_Point_2;
    typedef CGAL::Polygon_2<CGAL_Kernel> CGAL_Polygon_2;

    namespace CGAL_PS = CGAL::Polyline_simplification_2;
    typedef CGAL_PS::Stop_below_count_ratio_threshold CGAL_PS_Stop_CountRatio;
    typedef CGAL_PS::Stop_above_cost_threshold CGAL_PS_Stop_Cost;
    typedef CGAL_PS::Squared_distance_cost CGAL_PS_Cost;

    /*
     * 1) Extract from gameinfo: dimensions, buildability, pathability, heightmap
     **/
    void Map_BuildBasics( const GameInfo & info, size_t & width_out, size_t & height_out, Array2<uint64_t>& flags_out, Array2<Real>& heightmap_out )
    {
      width_out = info.width;
      height_out = info.height;

      flags_out.resize( width_out, height_out );
      heightmap_out.resize( width_out, height_out );

      for ( size_t x = 0; x < width_out; x++ )
        for ( size_t y = 0; y < height_out; y++ )
        {
          uint64_t flags = 0;
          if ( utils::placement( info, x, y ) )
            flags |= MapFlag_Buildable;
          if ( ( flags & MapFlag_Buildable ) || utils::pathable( info, x, y ) )
            flags |= MapFlag_Walkable;

          flags_out[x][y] = flags;

          heightmap_out[x][y] = utils::terrainHeight( info, x, y );
        }
    }

    /*
     * 2) Extract from pathability map: contours and connected components
     **/
    void Map_ProcessContours( Array2<uint64_t> flags_in, Array2<int>& labels_out, ComponentVector & components_out )
    {
      auto width = (int16_t)flags_in.width();
      auto height = (int16_t)flags_in.height();

      auto image_input = (uint8_t*)::malloc( width * height );

      int16_t lbl_w = width;
      int16_t lbl_h = height;
      blob_algo::label_t* lbl_out = nullptr;
      blob_algo::blob_t* blob_out = nullptr;

      int blob_count = 0;
      for ( size_t y = 0; y < height; y++ )
        for ( size_t x = 0; x < width; x++ )
        {
          auto idx = ( y * width ) + x;
          image_input[idx] = ( ( flags_in[x][y] & MapFlag_Walkable ) ? 0xFF : 0x00 );
        }

      if ( blob_algo::find_blobs( 0, 0, width, height, image_input, width, height, &lbl_out, &lbl_w, &lbl_h, &blob_out, &blob_count, 1 ) != 1 )
        HIVE_EXCEPT( "find_blobs failed" );

      labels_out.resize( lbl_w, lbl_h );

      for ( size_t y = 0; y < lbl_h; y++ )
        for ( size_t x = 0; x < lbl_w; x++ )
        {
          auto idx = ( y * lbl_w ) + x;
          labels_out[x][y] = lbl_out[idx];
        }

      auto fnConvertContour = []( blob_algo::contour_t& in, Contour& out )
      {
        for ( int i = 0; i < in.count; i++ )
          out.emplace_back(  in.points[i * sizeof( int16_t )], in.points[( i * sizeof( int16_t ) ) + 1]  );
      };

      for ( int i = 0; i < blob_count; i++ )
      {
        MapComponent comp;
        comp.label = blob_out[i].label;
        fnConvertContour( blob_out[i].external, comp.contour );
        for ( int j = 0; j < blob_out[i].internal_count; j++ )
        {
          Contour cnt;
          fnConvertContour( blob_out[i].internal[j], cnt );
          comp.holes.push_back( cnt );
        }
        components_out.push_back( comp );
      }

      ::free( image_input );
      ::free( lbl_out );

      blob_algo::destroy_blobs( blob_out, blob_count );
    }

    Polygon convertCgalPoly( CGAL_Polygon_2& in )
    {
      Polygon out;
      for ( auto vi = in.vertices_begin(); vi != in.vertices_end(); ++vi )
        out.emplace_back( (float)vi->x(), (float)vi->y() );
      return out;
    }

    /*
     * 3) Turn contours and components into clean polygons
     **/
    void Map_ComponentPolygons( ComponentVector& components_in, PolygonComponentVector& polygons_out, bool simplify, double simplify_stop_cost )
    {
      for ( auto& component : components_in )
      {
        PolygonComponent out_comp;
        out_comp.label = component.label;

        vector<CGAL_Point_2> points;
        for ( auto& pt : component.contour )
          points.emplace_back(  (double)pt.x, (double)pt.y  );

        CGAL_Polygon_2 contourPoly( points.begin(), points.end() );

        if ( simplify )
          contourPoly = CGAL_PS::simplify( contourPoly, CGAL_PS::Squared_distance_cost(), CGAL_PS_Stop_Cost( simplify_stop_cost ) );

        out_comp.contour = convertCgalPoly( contourPoly );

        for ( auto& hole : component.holes )
        {
          points.clear();
          for ( auto& pt : hole )
            points.emplace_back( (double)pt.x, (double)pt.y );

          CGAL_Polygon_2 holePoly( points.begin(), points.end() );

          if ( simplify )
            holePoly = CGAL_PS::simplify( holePoly, CGAL_PS::Squared_distance_cost(), CGAL_PS_Stop_Cost( simplify_stop_cost ) );

          out_comp.holes.push_back( convertCgalPoly( holePoly ) );
        }

        polygons_out.push_back( out_comp );
      }
    }

    void Map_FindResourceClusters( const sc2::ObservationInterface& observation, vector<UnitVector>& clusters_out, size_t minClusterSize, Real maxResourceDistance )
    {
      vector<UnitVector> potentialClusters;

      for ( auto& mineral : observation.GetUnits( Unit::Alliance::Neutral, utils::isMineral ) )
      {
        bool foundCluster = false;
        for ( auto& cluster : potentialClusters )
        {
          Real dist = Vector2( mineral.pos ).distance( cluster.center() );
          if ( dist <= maxResourceDistance )
          {
            Real groundDist = dist;
            if ( groundDist >= 0.0f && groundDist < maxResourceDistance )
            {
              cluster.push_back( mineral );
              foundCluster = true;
              break;
            }
          }
        }
        if ( !foundCluster )
        {
          potentialClusters.emplace_back();
          potentialClusters.back().push_back( mineral );
        }
      }

      for ( auto& geyser : observation.GetUnits( Unit::Alliance::Neutral, utils::isGeyser ) )
      {
        for ( auto& cluster : potentialClusters )
        {
          float groundDist = Vector2( geyser.pos ).distance( cluster.center() );
          if ( groundDist >= 0.0f && groundDist <= maxResourceDistance )
          {
            cluster.push_back( geyser );
            break;
          }
        }
      }

      for ( auto& cluster : potentialClusters )
        if ( cluster.size() >= minClusterSize )
          clusters_out.push_back( cluster );
    }

  }

}