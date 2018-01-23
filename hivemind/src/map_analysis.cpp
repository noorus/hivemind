#pragma warning(disable: 4267)
#include "stdafx.h"
#include "map.h"
#include "map_analysis.h"
#include "utilities.h"
#include "blob_algo.h"
#include "exception.h"

namespace hivemind {

  namespace Analysis {

    /*
     * 1) Extract from gameinfo: dimensions, buildability, pathability, heightmap
     **/
    void Map_BuildBasics( const sc2::ObservationInterface& observation, size_t& width_out, size_t& height_out, Array2<uint64_t>& flags_out, Array2<Real>& heightmap_out )
    {
      const GameInfo& info = observation.GetGameInfo();

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

          // naive; works for now but maybe improve later
          if ( ( flags & MapFlag_Walkable ) && !( flags & MapFlag_Buildable ) )
            flags |= ( MapFlag_Ramp | MapFlag_NearRamp );

          flags_out[x][y] = flags;

          heightmap_out[x][y] = utils::terrainHeight( info, x, y );
        }

      // second pass to mark inner walkables
      for ( size_t x = 0; x < width_out; x++ )
        for ( size_t y = 0; y < height_out; y++ )
          if ( ( flags_out[x][y] & MapFlag_Walkable ) && x > 2 && y > 2 && x <= ( width_out - 1 ) && y <= ( height_out - 1 ) )
          {
            // up to 2 tiles around me
            uint64_t aroundANDed = (
              flags_out[x - 1][y] & flags_out[x + 1][y] & flags_out[x][y - 1] & flags_out[x][y + 1] &
              flags_out[x - 2][y] & flags_out[x + 2][y] & flags_out[x][y - 2] & flags_out[x][y + 2] &
              flags_out[x - 1][y - 1] & flags_out[x + 1][y - 1] & flags_out[x - 1][y + 1] & flags_out[x + 1][y + 1]
            );
            uint64_t aroundORed = (
              flags_out[x - 1][y] | flags_out[x + 1][y] | flags_out[x][y - 1] | flags_out[x][y + 1] |
              flags_out[x - 2][y] | flags_out[x + 2][y] | flags_out[x][y - 2] | flags_out[x][y + 2] |
              flags_out[x - 1][y - 1] | flags_out[x + 1][y - 1] | flags_out[x - 1][y + 1] | flags_out[x + 1][y + 1]
            );
            if ( aroundANDed & MapFlag_Walkable )
              flags_out[x][y] |= MapFlag_InnerWalkable;
            if ( aroundORed & MapFlag_Ramp )
              flags_out[x][y] |= MapFlag_NearRamp;
          }

      // Mark vespene geysers.
      for ( auto geyser : observation.GetUnits( Unit::Alliance::Neutral ) )
      {
        if ( !utils::isGeyser( geyser ) )
          continue;

        int x = static_cast<int>(geyser->pos.x);
        int y = static_cast<int>(geyser->pos.y);
        for(int i = -1; i <= 1; ++i)
        {
          for(int j = -1; j <= 1; ++j)
          {
            flags_out[x + i][y + j] |= MapFlag_VespeneGeyser;
          }
        }
      }
    }

    /*
     * 2) Extract from pathability map: contours and connected components
     **/
    void Map_ProcessContours( Array2<uint64_t>& flags_in, Array2<int>& labels_out, ComponentVector & components_out )
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

    Polygon util_clipperPathToCleanPolygon( ClipperLib::Path& path, const bool simplify = false )
    {
      ClipperLib::CleanPolygon( path );

      ClipperLib::Paths contourSimpleOut;
      ClipperLib::SimplifyPolygon( path, contourSimpleOut );

      // We discard any extra polygons here, should probably log a message about them

      /*for ( size_t i = 0; i < contourSimpleOut.size(); i++ )
        printf( "util_clipperPathToCleanPolygon: poly %d, size %d\r\n", i, contourSimpleOut[i].size() );*/

      if ( contourSimpleOut.empty() )
        return Polygon();

      auto toSimplify = util_clipperPathToBoostPolygon( contourSimpleOut[0] );

      if ( !simplify )
        return util_boostPolygonToPolygon( toSimplify );

      BoostPolygon simplified;
      boost::geometry::simplify( toSimplify, simplified, 1.0 );

      if ( !boost::geometry::is_simple( simplified ) )
        HIVE_EXCEPT( "Simplified contour polygon is not simple!" );

      return util_boostPolygonToPolygon( simplified );
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

        auto clipperContour = util_contourToClipperPath( component.contour );
        out_comp.contour = util_clipperPathToCleanPolygon( clipperContour );

        if ( out_comp.contour.empty() )
          continue;

        for ( auto& hole : component.holes )
        {
          if ( hole.empty() )
            continue;

          auto clipperHole = util_contourToClipperPath( hole );
          out_comp.contour.holes.push_back( util_clipperPathToCleanPolygon( clipperHole ) );
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
      overall.contour.holes.push_back( largestCopy );
      obstacles_out.push_back( overall );
      for ( auto& hole : largestComponent->contour.holes )
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
        if ( *it == maxX )
          break;
        VoronoiPoint point1( *it, y ); ++it;
        VoronoiPoint point2( maxX, y );
        if ( it != border.end() )
        {
          point2.x( *it );
          ++it;
        }
        segments.emplace_back( point1, point2 );
        rtreeSegments.push_back( std::make_pair( BoostSegment( BoostPoint( point1.x(), point1.y() ), BoostPoint( point2.x(), point2.y() ) ), idPoint++ ) );
      }
    }

    static const Real DIFF_COEFICIENT = 0.21f; // 0.31f; // relative difference to consider a change between region<->chokepoint
    static const Real MIN_NODE_DIST = 6.0f; //0.25f; // 7; // minimum distance between nodes
    static const Real MIN_REGION_OBST_DIST = 2.5f; // 2.5f; // 1 = 9.7; // minimum distance to object to be considered as a region

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
        if ( poly.contour.empty() )
          continue;

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
        for ( auto& hole : poly.contour.holes )
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
      struct ParentNode {
        RegionNodeID id;
        bool isMaximal;
        ParentNode(): id( 0 ), isMaximal( false ) {}
        ParentNode( RegionNodeID _id, bool _isMaximal ): id( _id ), isMaximal( _isMaximal ) {}
      };

      // container to mark visited nodes, and parent list
      vector<bool> visited( graph.nodes.size() );
      vector<ParentNode> parentNodes( graph.nodes.size() );
      vector<RegionNodeID> lastValid( graph.nodes.size() );

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
        if ( graph.adjacencyList[id].size() == 1 )
        {
          graph.markNodeAsRegion( id );
          visited[id] = true;
          ParentNode parentNode( id, true );
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
        RegionNodeID v0 = nodeToVisit.top();
        nodeToVisit.pop();

        ParentNode parentNode = parentNodes[v0];
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
          }
          else // we reached a connection between visited paths
          {
            RegionNodeID v0Parent, v1Parent;

            // get right parent of v0
            if ( graph.nodeType[v0] == RegionGraph::NodeType_Region || graph.nodeType[v0] == RegionGraph::NodeType_Chokepoint )
              v0Parent = v0;
            else
              v0Parent = parentNodes[v0].id;

            // get right parent of v1
            if ( graph.nodeType[v1] == RegionGraph::NodeType_Region || graph.nodeType[v1] == RegionGraph::NodeType_Chokepoint )
              v1Parent = v1;
            else
              v1Parent = parentNodes[v1].id;

            RegionNodeID isMaximal0 = false;
            if ( graph.nodeType[v0Parent] == RegionGraph::NodeType_Region ) isMaximal0 = true;
            RegionNodeID isMaximal1 = false;
            if ( graph.nodeType[v1Parent] == RegionGraph::NodeType_Region ) isMaximal1 = true;

            // if the connected path between choke-region nodes is too close, remove choke
            if ( isMaximal0 != isMaximal1 && graph.nodes[v0Parent].distance( graph.nodes[v1Parent] ) < MIN_NODE_DIST )
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
    void Map_GetChokepointSides( const RegionGraph& graph, const bgi::rtree<BoostSegmentI, bgi::quadratic<16>>& rtree, std::map<RegionNodeID, Chokepoint>& chokepointSides )
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

    inline BoostPoint getMidpoint( BoostPoint a, BoostPoint b )
    {
      boost::geometry::subtract_point( a, b );
      boost::geometry::divide_value( a, 2 );
      boost::geometry::add_point( a, b );
      return a;
    }

    void extendLine( BoostPoint& a, BoostPoint& b )
    {
      BoostPoint extendCenter = getMidpoint( a, b );
      // translate to extend
      boost::geometry::subtract_point( a, extendCenter );
      boost::geometry::subtract_point( b, extendCenter );
      // extend
      boost::geometry::multiply_value( a, 1.1 );
      boost::geometry::multiply_value( b, 1.1 );
      // translate back
      boost::geometry::add_point( a, extendCenter );
      boost::geometry::add_point( b, extendCenter );
    }

    void convertPoly( const Polygon& in, BoostPolygon::ring_type& out )
    {
      for ( auto& pt : in )
        out.emplace_back(pt.x,pt.y);
    }

    void util_clipAreasToRegions( ClipperLib::Paths& areas, ClipperLib::Paths& holes, ClipperLib::Paths& cutters, PolygonVector& out )
    {
      // combine holes & cutters (union), then cut areas with the result (difference)
      ClipperLib::Clipper clipper;

      clipper.AddPaths( holes, ClipperLib::ptSubject, true );
      clipper.AddPaths( cutters, ClipperLib::ptClip, true );

      ClipperLib::Paths cutUnion;
      clipper.Execute( ClipperLib::ctUnion, cutUnion, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd );

      clipper.Clear();

      clipper.AddPaths( areas, ClipperLib::ptSubject, true );
      clipper.AddPaths( cutUnion, ClipperLib::ptClip, true );

      ClipperLib::Paths solution;
      clipper.Execute( ClipperLib::ctDifference, solution, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd );

      for ( auto& path : solution )
      {
        auto poly = util_clipperPathToCleanPolygon( path, true );
        out.push_back( poly );
      }
    }

    void Map_MakeRegions( const PolygonComponentVector& polygons, const RegionChokesMap& chokepointSides, Array2<uint64_t>& flagsmap, size_t width, size_t height, RegionVector& regions, Array2<int>& regionLabelMap, const RegionGraph& graph )
    {
      PolygonVector regionPolygons;

      BoostMultiPoly input;
      for ( auto& poly : polygons )
      {
        BoostPolygon output;
        convertPoly( poly.contour, output.outer() );
        input.push_back( output );
      }

      BoostMultiLine chokeLines;

      for ( auto& choke : chokepointSides )
      {
        BoostPoint a( choke.second.side1.x, choke.second.side1.y );
        BoostPoint b( choke.second.side2.x, choke.second.side2.y );
        extendLine( a, b );
        std::vector<BoostPoint> line = { a, b };
        BoostLine chokeLine;
        boost::geometry::assign_points( chokeLine, line );
        chokeLines.push_back( chokeLine );
      }

      const double buffer_distance = 0.25;
      boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy( buffer_distance );
      boost::geometry::strategy::buffer::join_miter join_strategy;
      boost::geometry::strategy::buffer::end_flat end_strategy;
      boost::geometry::strategy::buffer::point_square circle_strategy;
      boost::geometry::strategy::buffer::side_straight side_strategy;

      BoostMultiPoly cutPolygons;

      boost::geometry::buffer( chokeLines, cutPolygons,
        distance_strategy, side_strategy,
        join_strategy, end_strategy, circle_strategy );

      BoostMultiPoly holePolygons;

      for ( auto& poly : polygons )
      {
        for ( auto& hole : poly.contour.holes )
        {
          BoostPolygon holepoly;
          for ( auto& pt : hole )
            boost::geometry::append( holepoly, BoostPoint( pt.x, pt.y ) );
          holePolygons.push_back( holepoly );
        }
      }

      ClipperLib::Paths clipperAreas = util_boostMultiPolyToClipperPaths( input );
      ClipperLib::Paths clipperCutPolygons = util_boostMultiPolyToClipperPaths( cutPolygons );
      ClipperLib::Paths clipperHolePolygons = util_boostMultiPolyToClipperPaths( holePolygons );

      util_clipAreasToRegions( clipperAreas, clipperHolePolygons, clipperCutPolygons, regionPolygons );

      for ( auto& regionPoly : regionPolygons )
      {
        auto region = new Region();
        region->label_ = (int)regions.size();
        region->polygon_ = regionPoly;
        regions.push_back( region );
      }

      for ( size_t y = 0; y < height; y++ )
        for ( size_t x = 0; x < width; x++ )
        {
          if ( !flagsmap[x][y] & MapFlag_Walkable )
            continue;
          Vector2 pt( (Real)x, (Real)y );
          for ( size_t i = 0; i < regions.size(); i++ )
            if ( regions[i]->polygon_.contains( pt ) )
            {
              regionLabelMap[x][y] = (int)i;
              break;
            }
        }

      /*std::map<RegionNodeID, Region*> nodeToRegion;
      for ( const auto& id : graph.regionNodes )
      {
        auto label = regionLabelMap[(int)graph.nodes[id].x][(int)graph.nodes[id].y];
        if ( label < 0 || label > regions.size() )
          HIVE_EXCEPT( "Label for region node out of bounds" );
        auto region = regions[label];
        region->opennessDistance_ = graph.minDistToObstacle[id];
        region->opennessPoint_ = graph.nodes[id];
        nodeToRegion.emplace( id, region );
      }

      std::map<RegionNodeID, ChokeSides*> nodeToChoke;
      for ( const auto& id : graph.chokeNodes )
      {
        auto it = graph.adjacencyList[id].begin();
        auto region1 = nodeToRegion[*it];
        it++;
        auto region2 = nodeToRegion[*it];
        // auto side1 = chokepointSides[id].side1;
        // auto side2 = chokepointSides[id].side2;
      }*/
    }

    /*
    * 11) Find resource clusters to make base locations
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

    void Map_CacheClosestRegions( RegionVector& regions, Array2<int>& regionMap, Array2<int>& closest_out )
    {
      auto width = closest_out.width();
      auto height = closest_out.height();

      for ( int x = 0; x < width; x++ )
        for ( int y = 0; y < height; y++ )
        {
          if ( regionMap[x][y] >= 0 )
            closest_out[x][y] = regionMap[x][y];
          else
          {
            Vector2 point( (Real)x, (Real)y );
            Region* bestReg = nullptr;
            Real bestDist = std::numeric_limits<Real>::max();
            for ( auto region : regions )
            {
              auto dist = region->polygon_.distanceTo( point, true );
              if ( dist < bestDist )
              {
                bestDist = dist;
                bestReg = region;
              }
              if ( bestDist == 0.0f )
                break;
            }
            if ( !bestReg )
              HIVE_EXCEPT( "No closest region found" );
            closest_out[x][y] = bestReg->label_;
          }
        }
    }

    /*
    * xx) Add tile flags based on processed base locations
    **/
    void Map_MarkBaseTiles( Array2<uint64_t>& flags_out, const BaseLocationVector & locations )
    {
      auto width = flags_out.width();
      auto height = flags_out.height();

      for ( int x = 0; x < width; x++ )
        for ( int y = 0; y < height; y++ )
        {
          for ( auto& loc : locations )
          {
            if ( loc.isInResourceBox( x, y ) )
              flags_out[x][y] |= MapFlag_ResourceBox;
            if ( loc.overlapsMainFootprint( x, y ) )
              flags_out[x][y] |= ( MapFlag_StartLocation | MapFlag_NearStartLocation );
          }
        }

      // second pass to mark "near"-tiles
      for ( int x = 0; x < width; x++ )
        for ( int y = 0; y < height; y++ )
          if ( ( flags_out[x][y] & MapFlag_Walkable ) && x > 2 && y > 2 && x <= ( width - 1 ) && y <= ( height - 1 ) )
          {
            // up to 2 tiles around me
            uint64_t aroundORed = (
              flags_out[x - 1][y] | flags_out[x + 1][y] | flags_out[x][y - 1] | flags_out[x][y + 1] |
              flags_out[x - 2][y] | flags_out[x + 2][y] | flags_out[x][y - 2] | flags_out[x][y + 2] |
              flags_out[x - 1][y - 1] | flags_out[x + 1][y - 1] | flags_out[x - 1][y + 1] | flags_out[x + 1][y + 1]
              );
            if ( aroundORed & MapFlag_StartLocation )
              flags_out[x][y] |= MapFlag_NearStartLocation;
          }
    }

  }

}