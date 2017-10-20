#include "stdafx.h"
#include "map.h"
#include "map_analysis.h"
#include "utilities.h"
#include "blob_algo.h"
#include "exception.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polyline_simplification_2/simplify.h>
#include <CGAL/Polyline_simplification_2/Hybrid_squared_distance_cost.h>
#include <CGAL/Segment_Delaunay_graph_traits_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Cartesian.h>

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

    /*
    * Utility to convert CGAL polygon to our Polygon
    **/
    inline Polygon util_cgalPolyToPoly( CGAL_Polygon_2& in )
    {
      Polygon out;
      for ( auto vi = in.vertices_begin(); vi != in.vertices_end(); ++vi )
        out.emplace_back( (float)vi->x(), (float)vi->y() );
      return out;
    }

    /*
    * Utility to convert our Polygon to CGAL polygon
    **/
    inline CGAL_Polygon_2 util_polyToCgalPoly( Polygon& in )
    {
      CGAL_Polygon_2 out;
      for ( auto& pt : in )
        out.push_back( CGAL_Point_2( (double)pt.x, (double)pt.y ) );
      return out;
    }

    /*
     * Utility to try to naively clean up some self-loops in the edges
     **/
    void util_cleanupPolygon( CGAL_Polygon_2& poly )
    {
      auto fnEncodePoint = []( const Point2DI& pt ) -> uint64_t
      { return ( ( (uint64_t)pt.x ) << 32 ) | ( (uint64_t)pt.y ); };

      using ptmap = std::map<uint64_t, size_t>;
      ptmap existingPointsMap;
      bool reloop = true;
      while ( reloop )
      {
        existingPointsMap.clear();
        reloop = false;
        for ( size_t i = 0; i < poly.size(); i++ )
        {
          auto pt = Point2DI( (int)poly[i].x(), (int)poly[i].y() );
          if ( existingPointsMap.count( fnEncodePoint( pt ) ) )
          {
            auto previousIndex = existingPointsMap[fnEncodePoint( pt )];
            poly.erase( poly.vertices_begin() + previousIndex, poly.vertices_begin() + i );
            reloop = true;
            break;
          } else
            existingPointsMap[fnEncodePoint( pt )] = i;
        }
      }
    }

    /*
     * 3) Turn contours and components into clean polygons
     **/
    void Map_ComponentPolygons( ComponentVector& components_in, PolygonComponentVector& polygons_out, bool simplify, double simplify_distance_cost, double simplify_stop_cost )
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
          contourPoly = CGAL_PS::simplify( contourPoly, CGAL_PS::Hybrid_squared_distance_cost<double>( simplify_distance_cost ), CGAL_PS_Stop_Cost( simplify_stop_cost ) );

        util_cleanupPolygon( contourPoly );
        if ( !contourPoly.is_simple() )
          HIVE_EXCEPT( "Cleaned component contour polygon is not simple" );

        out_comp.contour = util_cgalPolyToPoly( contourPoly );

        for ( auto& hole : component.holes )
        {
          points.clear();
          for ( auto& pt : hole )
            points.emplace_back( (double)pt.x, (double)pt.y );

          CGAL_Polygon_2 holePoly( points.begin(), points.end() );

          if ( simplify )
            holePoly = CGAL_PS::simplify( holePoly, CGAL_PS::Hybrid_squared_distance_cost<double>( simplify_distance_cost ), CGAL_PS_Stop_Cost( simplify_stop_cost ) );

          util_cleanupPolygon( holePoly );
          if ( !holePoly.is_simple() )
            HIVE_EXCEPT( "Cleaned hole contour polygon is not simple" );

          out_comp.holes.push_back( util_cgalPolyToPoly( holePoly ) );
        }

        polygons_out.push_back( out_comp );
      }
    }

    /*
     * 4) Make an inverted copy of the polygons (i.e. walkables to non-walkables) for Voronoi diagram generation
     **/
    void Map_InvertPolygons( const PolygonComponentVector & walkable_in, PolygonComponentVector & obstacles_out, const Rect2& playableArea, const Vector2& dimensions )
    {
      // contour = purple
      // holes = red
      Real largestArea = 0.0f;
      const Polygon* largestPoly = nullptr;
      const PolygonComponent* largestComponent = nullptr;
      vector<const Polygon*> potential;
      for ( auto& comp : walkable_in )
      {
        if ( comp.contour.area() > largestArea )
        {
          largestArea = comp.contour.area();
          if ( largestPoly )
            potential.push_back( largestPoly );
          largestPoly = &comp.contour;
          largestComponent = &comp;
        } else
        {
          bool ok = false;
          for ( auto& pt : comp.contour )
            if ( pt >= playableArea.min_ && pt <= playableArea.max_ )
            {
              ok = true;
              break;
            }
          if ( ok )
            potential.push_back( &comp.contour );
        }
      }
      if ( !largestPoly )
        return;
      PolygonComponent overall;
      overall.contour.push_back( Vector2( 0.0f, 0.0f ) );
      overall.contour.push_back( Vector2( dimensions.x, 0.0f ) );
      overall.contour.push_back( dimensions );
      overall.contour.push_back( Vector2( 0.0f, dimensions.y ) );
      overall.label = 0; // unwalkable
      Polygon largestCopy = ( *largestPoly );
      overall.holes.push_back( largestCopy );
      obstacles_out.push_back( overall );
      for ( auto& hole : largestComponent->holes )
      {
        PolygonComponent innerComp;
        innerComp.contour = hole;
        innerComp.label = 0; // unwalkable
        obstacles_out.push_back( innerComp );
      }
      for ( auto inner : potential )
      {
        PolygonComponent innerComp;
        innerComp.contour = ( *inner );
        innerComp.label = 0; // unwalkable
        obstacles_out.push_back( innerComp );
      }
    }

    void addVerticalBorder( std::vector<VoronoiSegment>& segments, std::vector<BoostSegmentI>& rtreeSegments, size_t& idPoint,
      const std::set<int>& border, int x, int maxY )
    {
      auto it = border.begin();
      {
        VoronoiPoint point1( x, 0 );
        VoronoiPoint point2( x, maxY );
        if ( it != border.end() )
        {
          point2.y( *it ); ++it;
        }
        segments.emplace_back( point1, point2 );
        rtreeSegments.push_back( std::make_pair( BoostSegment( BoostPoint( point1.x(), point1.y() ), BoostPoint( point2.x(), point2.y() ) ), idPoint++ ) );
      }
      for ( it; it != border.end();)
      {
        if ( *it == maxY ) break;
        VoronoiPoint point1( x, *it ); ++it;
        VoronoiPoint point2( x, maxY );
        if ( it != border.end() )
        {
          point2.y( *it ); ++it;
        }
        segments.emplace_back( point1, point2 );
        rtreeSegments.push_back( std::make_pair( BoostSegment( BoostPoint( point1.x(), point1.y() ), BoostPoint( point2.x(), point2.y() ) ), idPoint++ ) );
      }
    }

    void addHorizontalBorder( std::vector<VoronoiSegment>& segments, std::vector<BoostSegmentI>& rtreeSegments, size_t& idPoint,
      const std::set<int>& border, int y, int maxX )
    {
      auto it = border.begin();
      {
        VoronoiPoint point1( 0, y );
        VoronoiPoint point2( maxX, y );
        if ( it != border.end() )
        {
          point2.x( *it ); ++it;
        }
        segments.emplace_back( point1, point2 );
        rtreeSegments.push_back( std::make_pair( BoostSegment( BoostPoint( point1.x(), point1.y() ), BoostPoint( point2.x(), point2.y() ) ), idPoint++ ) );
      }
      for ( it; it != border.end();)
      {
        if ( *it == maxX ) break;
        VoronoiPoint point1( *it, y ); ++it;
        VoronoiPoint point2( maxX, y );
        if ( it != border.end() )
        {
          point2.x( *it ); ++it;
        }
        segments.emplace_back( point1, point2 );
        rtreeSegments.push_back( std::make_pair( BoostSegment( BoostPoint( point1.x(), point1.y() ), BoostPoint( point2.x(), point2.y() ) ), idPoint++ ) );
      }
    }

    static const Real DIFF_COEFICIENT = 0.31f; // relative difference to consider a change between region<->chokepoint
    static const Real MIN_NODE_DIST = 0.25f; // 7; // minimum distance between nodes
    static const Real MIN_REGION_OBST_DIST = 2.5f; // 1 = 9.7; // minimum distance to object to be considered as a region

    /*
     * 5) Turn walkable areas (by way of non-walkable area polygons) to a Voronoi diagram
     * See "Improving Terrain Analysis and Applications to RTS Game AI" (Uriarte, Ontañón, AIIDE 16) & BWTA2
     **/
    void Map_MakeVoronoi( const GameInfo & info, PolygonComponentVector & polygons_in, Array2<int>& labels_in, RegionGraph& graph, bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree )
    {
      std::vector<VoronoiSegment> segments;
      std::vector<BoostSegmentI> rtreeSegments;
      size_t idPoint = 0;

      std::set<int> rightBorder, leftBorder, topBorder;
      int maxX = info.width - 1;
      int maxY = info.height - 1;

      for ( const auto& poly: polygons_in )
      {
        size_t lastPoint = poly.contour.size() - 1;
        for ( size_t i = 0, j = 1; i < lastPoint; ++i, ++j )
        {
          if ( poly.contour[i].x == 0 && poly.contour[j].x == 0 )
          {
            leftBorder.insert( (int)poly.contour[i].y );
            leftBorder.insert( (int)poly.contour[j].y );
          } else if ( poly.contour[i].x == maxX && poly.contour[j].x == maxX )
          {
            rightBorder.insert( (int)poly.contour[i].y );
            rightBorder.insert( (int)poly.contour[j].y );
          }
          if ( poly.contour[i].y == 0 && poly.contour[j].y == 0 )
          {
            topBorder.insert( (int)poly.contour[i].x );
            topBorder.insert( (int)poly.contour[j].x );
          }
          segments.emplace_back( VoronoiPoint( poly.contour[j].x, poly.contour[j].y ), VoronoiPoint( poly.contour[i].x, poly.contour[i].y ) );

          rtreeSegments.push_back( std::make_pair( BoostSegment( BoostPoint( poly.contour[j].x, poly.contour[j].y ),
            BoostPoint( poly.contour[i].x, poly.contour[i].y ) ), idPoint++ ) );
        }
        for ( auto& hole : poly.holes )
        {
          size_t lastPoint = hole.size() - 1;
          for ( size_t i = 0, j = 1; i < lastPoint; ++i, ++j )
          {
            segments.emplace_back( VoronoiPoint( hole[j].x, hole[j].y ), VoronoiPoint( hole[i].x, hole[i].y ) );

            rtreeSegments.push_back( std::make_pair( BoostSegment( BoostPoint( hole[j].x, hole[j].y ),
              BoostPoint( hole[i].x, hole[i].y ) ), idPoint++ ) );
          }
        }
      }

      addVerticalBorder( segments, rtreeSegments, idPoint, leftBorder, 0, maxY );
      addVerticalBorder( segments, rtreeSegments, idPoint, rightBorder, maxX, maxY );
      addHorizontalBorder( segments, rtreeSegments, idPoint, topBorder, 0, maxX );

      BoostVoronoi voronoi;
      boost::polygon::construct_voronoi( segments.begin(), segments.end(), &voronoi );

      static const std::size_t VISITED_COLOR = 1;
      for ( const auto& edge : voronoi.edges() )
      {
        if ( !edge.is_primary() || !edge.is_finite() || edge.color() == VISITED_COLOR )
          continue;

        edge.twin()->color( VISITED_COLOR );

        const BoostVoronoi::vertex_type* v0 = edge.vertex0();
        const BoostVoronoi::vertex_type* v1 = edge.vertex1();

        if ( ::isnan( v0->x() ) || ::isnan( v0->y() ) || ::isnan( v1->x() ) || ::isnan( v1->y() ) )
          continue;

        Vector2 p0( (Real)v0->x(), (Real)v0->y() );
        Vector2 p1( (Real)v1->x(), (Real)v1->y() );

        if ( p0.x < 0 || p0.x > maxX || p0.y < 0 || p0.y > maxY || p1.x < 0 || p1.x > maxX || p1.y < 0 || p1.y > maxY )
        {
          // HIVE_EXCEPT( "Voronoi vertex found outside map. Probably wrong border segment generation" );
          continue;
        }
        //... is outside map or near border
        // 			if (p0.x < SKIP_NEAR_BORDER || p0.x > maxXborder || p0.y < SKIP_NEAR_BORDER || p0.y >maxYborder || 
        //				p1.x < SKIP_NEAR_BORDER || p1.x >maxXborder || p1.y < SKIP_NEAR_BORDER || p1.y >maxYborder) continue;

        // ... any of its endpoints is inside an obstacle
        if ( labels_in[(int)p0.x][(int)p0.y] == 0 || labels_in[(int)p1.x][(int)p1.y] == 0 ) // 0 = unwalkable
          continue;

        RegionNodeID v0ID = graph.addNode( v0, p0 );
        RegionNodeID v1ID = graph.addNode( v1, p1 );
        graph.addEdge( v0ID, v1ID );
      }

      bgi::rtree<BoostSegmentI, bgi::quadratic<16> > rtreeConstruct( rtreeSegments );
      rtree = rtreeConstruct;
      graph.minDistToObstacle.resize( graph.nodes.size() );
      for ( size_t i = 0; i < graph.nodes.size(); ++i )
      {
        std::vector<BoostSegmentI> returnedValues;
        BoostPoint pt( graph.nodes[i].x, graph.nodes[i].y );
        rtree.query( bgi::nearest( pt, 1 ), std::back_inserter( returnedValues ) );
        graph.minDistToObstacle[i] = boost::geometry::distance( returnedValues.front().first, pt );
      }
    }

    /*
     * 6) Prune down the messy Voronoi diagram to only the medial axes per every branch
     * See "Improving Terrain Analysis and Applications to RTS Game AI" (Uriarte, Ontañón, AIIDE 16) & BWTA2
     **/
    void Map_PruneVoronoi( RegionGraph & graph )
    {
      // get the list of all leafs (nodes with only one element in the adjacent list)
      std::queue<RegionNodeID> leaves;
      for ( size_t id = 0; id < graph.adjacencyList.size(); ++id )
        if ( graph.adjacencyList[id].size() == 1 )
          leaves.push( id );

      // using leafs as starting points, prune the RegionGraph
      while ( !leaves.empty() )
      {
        // pop first element
        RegionNodeID v0 = leaves.front();
        leaves.pop();

        if ( graph.adjacencyList[v0].empty() )
          continue;

        RegionNodeID v1 = *graph.adjacencyList[v0].begin();
        // remove node if it's too close to an obstacle, or parent is farther to an obstacle
        if ( graph.minDistToObstacle[v0] < MIN_REGION_OBST_DIST
          || graph.minDistToObstacle[v0] - 0.9 <= graph.minDistToObstacle[v1] )
        {
          graph.adjacencyList[v0].clear();
          graph.adjacencyList[v1].erase( v0 );

          if ( graph.adjacencyList[v1].empty() && graph.minDistToObstacle[v1] > MIN_REGION_OBST_DIST )
          {
            // isolated node
            graph.markNodeAsRegion( v1 );
          } else if ( graph.adjacencyList[v1].size() == 1 )
          { // keep pruning if only one child
            leaves.push( v1 );
          }
        }
      }
    }

    /*
     * 7) DetectNodes
     **/
    void Map_DetectNodes( RegionGraph & graph, const PolygonComponentVector& polygons )
    {
      struct parentNode_t {
        RegionNodeID id;
        bool isMaximal;
        parentNode_t(): id( 0 ), isMaximal( false ) {}
        parentNode_t( RegionNodeID _id, bool _isMaximal ): id( _id ), isMaximal( _isMaximal ) {}
      };

      // container to mark visited nodes, and parent list
      std::vector<bool> visited( graph.nodes.size() );
      std::vector<parentNode_t> parentNodes( graph.nodes.size() );
      std::vector<RegionNodeID> lastValid( graph.nodes.size() );

      auto fnEnoughDifference = []( const double& a, const double& b ) -> bool
      {
        double diff = math::abs( a - b );
        double largest = std::max( a, b );
        double minDiff = std::max( 2.0, largest * DIFF_COEFICIENT );
        return ( diff > minDiff );
      };

      std::stack<RegionNodeID> nodeToVisit;
      for ( size_t id = 0; id < graph.adjacencyList.size(); ++id )
      {
        // find a leaf
        if ( graph.adjacencyList[id].size() == 1 )
        {
          graph.markNodeAsRegion( id );
          visited[id] = true;
          parentNode_t parentNode( id, true );
          parentNodes[id] = parentNode;
          for ( const auto& v1 : graph.adjacencyList[id] )
          {
            nodeToVisit.push( v1 );
            visited[1] = true;
            parentNodes[v1] = parentNode;
          }
          break;
        }
      }

      while ( !nodeToVisit.empty() )
      {
        // pop first element
        RegionNodeID v0 = nodeToVisit.top();
        nodeToVisit.pop();
        // get parent
        parentNode_t parentNode = parentNodes[v0];
        if ( graph.adjacencyList[v0].size() != 2 )
        { // We found a leaf or intersection (region node)
          // if parent chokepoint and too close, delete parent
          if ( graph.nodeType[parentNode.id] == RegionGraph::NodeType_Chokepoint &&
            graph.nodes[v0].distance( graph.nodes[parentNode.id] ) < MIN_NODE_DIST )
          {
            graph.unmarkChokeNode( parentNode.id );
          }
          graph.markNodeAsRegion( v0 );
          // don't update next parent if current parent is a bigger region
          if ( graph.nodeType[parentNode.id] == RegionGraph::NodeType_Chokepoint ||
            graph.minDistToObstacle[v0] > graph.minDistToObstacle[parentNode.id] )
          {
            parentNode.id = v0; parentNode.isMaximal = true; // updating next parent
          }

        } else
        {
          // see if the node is a local minimal (chokepoint node)
          bool localMinimal = true;
          for ( const auto& v1 : graph.adjacencyList[v0] )
            if ( graph.minDistToObstacle[v0] > graph.minDistToObstacle[v1] )
            {
              localMinimal = false;
              break;
            }

          if ( localMinimal )
          {
            if ( !parentNode.isMaximal )
            {
              // choke to choke
              if ( graph.minDistToObstacle[v0] < graph.minDistToObstacle[parentNode.id] )
              {
                lastValid[v0] = parentNode.id;
                graph.unmarkChokeNode( parentNode.id );
                graph.markNodeAsChoke( v0 );
                parentNode.id = v0;
                parentNode.isMaximal = false;

              }
            }
            else
            {
              // region to choke
              auto approxDistance = graph.nodes[v0].distance( graph.nodes[parentNode.id] );
              bool enoughDistance = approxDistance >= MIN_NODE_DIST && approxDistance > graph.minDistToObstacle[parentNode.id];
              if ( fnEnoughDifference( graph.minDistToObstacle[v0], graph.minDistToObstacle[parentNode.id] ) || enoughDistance )
              {
                graph.markNodeAsChoke( v0 );
                parentNode.id = v0;
                parentNode.isMaximal = false;
              }
            }
          }
          else
          {
            // see if the node is a local maximal (region node)
            bool localMaximal = true;
            for ( const auto& v1 : graph.adjacencyList[v0] )
              if ( graph.minDistToObstacle[v0] < graph.minDistToObstacle[v1] )
              {
                localMaximal = false;
                break;
              }

            if ( localMaximal )
            {
              if ( graph.minDistToObstacle[v0] < MIN_REGION_OBST_DIST )
              {
			
              }
              else if ( parentNode.isMaximal )
              {
                // region to region
                if ( graph.minDistToObstacle[v0] > graph.minDistToObstacle[parentNode.id] )
                {
                  // only delete parent if it isn't an intersection
                  if ( graph.adjacencyList[parentNode.id].size() == 2 )
                    graph.unmarkRegionNode( parentNode.id );
                  graph.markNodeAsRegion( v0 );
                  parentNode.id = v0;
                  parentNode.isMaximal = true;
                }
              }
              else
              {
                // choke to region
                if ( fnEnoughDifference( graph.minDistToObstacle[v0], graph.minDistToObstacle[parentNode.id] ) )
                {
                  graph.markNodeAsRegion( v0 );
                  parentNode.id = v0;
                  parentNode.isMaximal = true;
                }
              }
            }
          }
        }

        // keep exploring unvisited neighbors
        for ( const auto& v1 : graph.adjacencyList[v0] )
        {
          if ( !visited[v1] )
          {
            nodeToVisit.push( v1 );
            visited[v1] = true;
            parentNodes[v1] = parentNode;
          } else
          {
            // we reached a connection between visited paths.

            RegionNodeID v0Parent, v1Parent;
            // get right parent of v0
            if ( graph.nodeType[v0] == RegionGraph::NodeType_Region || graph.nodeType[v0] == RegionGraph::NodeType_Chokepoint )
            {
              v0Parent = v0;
            } else v0Parent = parentNodes[v0].id;
            // get right parent of v1
            if ( graph.nodeType[v1] == RegionGraph::NodeType_Region || graph.nodeType[v1] == RegionGraph::NodeType_Chokepoint )
            {
              v1Parent = v1;
            } else v1Parent = parentNodes[v1].id;
            RegionNodeID isMaximal0 = false;
            if ( graph.nodeType[v0Parent] == RegionGraph::NodeType_Region ) isMaximal0 = true;
            RegionNodeID isMaximal1 = false;
            if ( graph.nodeType[v1Parent] == RegionGraph::NodeType_Region ) isMaximal1 = true;

            // 					if (!isMaximal0 && isMaximal1) LOG("Choke-region " << v0Parent << "-" << v1Parent);
            // 					if (isMaximal0 && !isMaximal1) LOG("Region-choke " << v0Parent << "-" << v1Parent);

            // if the connected path between choke-region nodes is too close, remove choke
            if ( isMaximal0 != isMaximal1 &&
              graph.nodes[v0Parent].distance( graph.nodes[v1Parent] ) < MIN_NODE_DIST )
            {
              RegionNodeID nodeToDelete = v0Parent;
              if ( isMaximal0 )  nodeToDelete = v1Parent;
              graph.unmarkChokeNode( nodeToDelete );
              if ( lastValid[nodeToDelete] != 0 )
              {
                graph.markNodeAsChoke( lastValid[nodeToDelete] );
              }
            } else
              // if two consecutive minimals, keep the min
              if ( !isMaximal0 && isMaximal0 == isMaximal1 && v0Parent != v1Parent )
              {
                RegionNodeID nodeToDelete = v0Parent;
                if ( graph.minDistToObstacle[v0Parent] < graph.minDistToObstacle[v1Parent] )
                {
                  nodeToDelete = v1Parent;
                }
                graph.unmarkChokeNode( nodeToDelete );
              }
          }
        }
      }
    }

    /*
    * 8) x
    **/
    void Map_SimplifyGraph( const RegionGraph & graph, RegionGraph & graphSimplified )
    {
      std::vector<bool> visited;
      visited.resize( graph.nodes.size() );
      std::vector<RegionNodeID> parentID;
      parentID.resize( graph.nodes.size() );

      RegionNodeID leafRegionId = 99;
      RegionNodeID newleafRegionId;
      for ( const auto& regionId : graph.regionNodes )
      {
        if ( leafRegionId == 99 && graph.adjacencyList[regionId].size() == 1 )
        {
          leafRegionId = regionId;
          visited[regionId] = true;
          newleafRegionId = graphSimplified.addNode( graph.nodes[regionId], graph.minDistToObstacle[regionId] );
          graphSimplified.markNodeAsRegion( newleafRegionId );
        }
        if ( graph.adjacencyList[regionId].empty() )
        {
          RegionNodeID newId = graphSimplified.addNode( graph.nodes[regionId], graph.minDistToObstacle[regionId] );
          graphSimplified.markNodeAsRegion( newId );
        }
      }

      std::stack<RegionNodeID> nodeToVisit;
      if ( leafRegionId != 99 )
        for ( const auto& v1 : graph.adjacencyList[leafRegionId] )
        {
          nodeToVisit.emplace( v1 );
          visited[v1] = true;
          parentID[v1] = newleafRegionId;
        }

      while ( !nodeToVisit.empty() )
      {
        RegionNodeID nodeId = nodeToVisit.top();
        nodeToVisit.pop();
        RegionNodeID parentId = parentID[nodeId];

        if ( graph.nodeType[nodeId] == RegionGraph::NodeType_Chokepoint )
        {
          RegionNodeID newId = graphSimplified.addNode( graph.nodes[nodeId], graph.minDistToObstacle[nodeId] );
          if ( newId != parentId )
          {
            graphSimplified.markNodeAsChoke( newId );
            graphSimplified.addEdge( newId, parentId );
            parentId = newId;
          }
        } else if ( graph.nodeType[nodeId] == RegionGraph::NodeType_Region )
        {
          RegionNodeID newId = graphSimplified.addNode( graph.nodes[nodeId], graph.minDistToObstacle[nodeId] );
          if ( newId != parentId )
          {
            graphSimplified.markNodeAsRegion( newId );
            graphSimplified.addEdge( newId, parentId );
            parentId = newId;
          }
        }
        for ( const auto& v1 : graph.adjacencyList[nodeId] )
        {
          if ( !visited[v1] )
          {
            nodeToVisit.emplace( v1 );
            parentID[v1] = parentId;
            visited[v1] = true;
          } else if ( parentID[v1] != parentID[nodeId] )
          {
            RegionNodeID n1 = parentID[v1];
            if ( graph.nodeType[v1] != RegionGraph::NodeType_None )
            {
              n1 = graphSimplified.addNode( graph.nodes[v1], graph.minDistToObstacle[v1] );
            }
            RegionNodeID n2 = parentID[nodeId];
            if ( graph.nodeType[nodeId] != RegionGraph::NodeType_None )
            {
              n2 = graphSimplified.addNode( graph.nodes[nodeId], graph.minDistToObstacle[nodeId] );
            }
            if ( n1 != n2 && graphSimplified.nodeType[n1] != RegionGraph::NodeType_None
              && graphSimplified.nodeType[n2] != RegionGraph::NodeType_None )
            {
              graphSimplified.addEdge( n1, n2 );
            }
          }
        }
      }
    }

    /*
    * 9) x
    **/
    void Map_MergeRegionNodes( RegionGraph & graph )
    {
      std::set<RegionNodeID> mergeRegions( graph.regionNodes );

      while ( !mergeRegions.empty() )
      {
        // pop first element
        RegionNodeID parent = *mergeRegions.begin();
        mergeRegions.erase( mergeRegions.begin() );

        for ( auto it = graph.adjacencyList[parent].begin(); it != graph.adjacencyList[parent].end();)
        {
          RegionNodeID child = *it;
          if ( parent == child )
          { // This is a self-loop, remove it
            it = graph.adjacencyList[parent].erase( it );
            continue;
          }
          if ( graph.nodeType[parent] == RegionGraph::NodeType_Region && graph.nodeType[child] == RegionGraph::NodeType_Region )
          {
            if ( graph.minDistToObstacle[parent] > graph.minDistToObstacle[child] )
            {
              graph.adjacencyList[parent].insert( graph.adjacencyList[child].begin(),
                graph.adjacencyList[child].end() );
              for ( const auto& v2 : graph.adjacencyList[child] )
              {
                graph.adjacencyList[v2].erase( child );
                if ( v2 == parent ) graph.adjacencyList[parent].erase( parent );
                else graph.adjacencyList[v2].insert( parent );
              }
              graph.adjacencyList[child].clear();
              graph.regionNodes.erase( child );
              it = graph.adjacencyList[parent].begin();
            } else
            {
              graph.adjacencyList[child].insert(
                graph.adjacencyList[parent].begin(),
                graph.adjacencyList[parent].end() );
              for ( const auto& v2 : graph.adjacencyList[parent] )
              {
                graph.adjacencyList[v2].erase( parent );
                if ( v2 == child ) graph.adjacencyList[child].erase( child );
                else graph.adjacencyList[v2].insert( child );
              }
              graph.adjacencyList[parent].clear();
              graph.regionNodes.erase( parent );
              mergeRegions.insert( child );
              break;
            }
          } else
          {
            ++it;
          }
        }
      }
    }

    Vector2 util_getProjectedPoint( const BoostPoint &p, const BoostSegment &s )
    {
      double dx = s.second.x() - s.first.x();
      double dy = s.second.y() - s.first.y();
      if ( dx != 0 || dy != 0 )
      {
        double t = ( ( p.x() - s.first.x() ) * dx + ( p.y() - s.first.y() ) * dy ) / ( dx * dx + dy * dy );
        if ( t > 1 )
        {
          return Vector2( static_cast<Real>( s.second.x() ), static_cast<Real>( s.second.y() ) );
        } else if ( t > 0 )
        {
          return Vector2( static_cast<Real>( s.first.x() + dx * t ), static_cast<Real>( s.first.y() + dy * t ) );
        }
      }
      return Vector2( static_cast<Real>( s.first.x() ), static_cast<Real>( s.first.y() ) );
    }

    /*
    * 10) Project the chokepoints from the graph
    **/
    void Map_GetChokepointSides( const RegionGraph & graph, const bgi::rtree<BoostSegmentI, bgi::quadratic<16>>& rtree, std::map<RegionNodeID, Chokepoint>& chokepointSides )
    {
      for ( const auto& id : graph.chokeNodes )
      {
        BoostPoint pt( graph.nodes[id].x, graph.nodes[id].y );
        Vector2 side1, side2;

        auto it = rtree.qbegin( bgi::nearest( pt, 10 ) );
        side1 = util_getProjectedPoint( pt, it->first );
        ++it;

        for ( it; it != rtree.qend(); ++it )
        {
          side2 = util_getProjectedPoint( pt, it->first );
          if ( ( ( side1.x <= pt.x() && side2.x >= pt.x() ) || ( side1.x >= pt.x() && side2.x <= pt.x() ) ) &&
            ( ( side1.y <= pt.y() && side2.y >= pt.y() ) || ( side1.y >= pt.y() && side2.y <= pt.y() ) ) )
          {
            break;
          }
        }
        chokepointSides.emplace( id, Chokepoint( side1, side2 ) );
      }
    }

    /*
    * 11) Finc resource clusters to make base locations
    **/
    void Map_FindResourceClusters( const sc2::ObservationInterface& observation, vector<UnitVector>& clusters_out, size_t minClusterSize, Real maxResourceDistance )
    {
      vector<UnitVector> tempClusters;
      for ( auto mineral : observation.GetUnits( Unit::Alliance::Neutral ) )
      {
        if ( !utils::isMineral( mineral ) )
          continue;

        UnitVector* bestCluster = nullptr;
        Real bestDistance = std::numeric_limits<Real>::max();

        for ( auto& cluster : tempClusters )
        {
          Real distance = utils::calculateCenter( cluster ).distance( mineral->pos );
          if ( distance >= 0.0f && distance < maxResourceDistance && distance < bestDistance )
          {
            bestCluster = &cluster;
            bestDistance = distance;
          }
        }

        if ( bestCluster )
          bestCluster->push_back( mineral );
        else {
          tempClusters.emplace_back();
          tempClusters.back().push_back( mineral );
        }
      }

      for ( auto geyser : observation.GetUnits( Unit::Alliance::Neutral ) )
      {
        if ( !utils::isGeyser( geyser ) )
          continue;

        UnitVector* bestCluster = nullptr;
        Real bestDistance = std::numeric_limits<Real>::max();

        for ( auto& cluster : tempClusters )
        {
          Real distance = utils::calculateCenter( cluster ).distance( geyser->pos );
          if ( distance >= 0.0f && distance < maxResourceDistance && distance < bestDistance )
          {
            bestCluster = &cluster;
            bestDistance = distance;
          }
        }

        if ( bestCluster )
          bestCluster->push_back( geyser );
      }

      clusters_out.clear();
      for ( auto& cluster : tempClusters )
      {
        if ( cluster.size() >= minClusterSize )
          clusters_out.push_back( cluster );
      }
    }

  }

}