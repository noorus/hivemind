#include "stdafx.h"
#include "map.h"
#include "bot.h"
#include "utilities.h"
#include "map_analysis.h"
#include "hive_geometry.h"
#include "baselocation.h"
#include "database.h"
#include "blob_algo.h"
#include "regiongraph.h"
#include "cache.h"
#include "pathfinding.h"
#include "hive_minheap.h"

namespace hivemind {

  GridGraph::GridGraph( Map& map ):
    width( (int)map.width() ),
    height( (int)map.height() )
  {
    grid.resize( width, height );
    for ( int x = 0; x < width; ++x )
      for ( int y = 0; y < height; ++y )
      {
        GridGraphNode node( x, y );
        node.valid = ( map.flagsMap_[x][y] & MapFlag_Walkable );
        node.closed = false;
        grid[x][y] = node;
      }
  }

  Real GridGraph::cost( const GridGraphNode& from, const GridGraphNode& to ) const
  {
    Real weight = 1.0f;

    if ( to.x != from.x && to.y != from.y )
      return ( weight * 1.41421f );
    else
      return weight;
  }

  inline uint64_t util_encodePoint( const Point2DI& pt )
  {
    return ( ( (uint64_t)pt.x ) << 32 ) | ( (uint64_t)pt.y );
  };

  inline Real util_diagonalDistanceHeuristic( const GridGraphNode& a, const MapPoint2& b )
  {
    auto dx = (Real)math::abs( b.x - a.x );
    auto dy = (Real)math::abs( b.y - a.y );
    Real D = 1.0f;
    Real D2 = math::sqrt( 2.0f );
    return ( D * ( dx + dy ) ) + ( ( D2 - ( 2.0f * D ) ) * std::min( dx, dy ) );
  }

  inline GridGraphNode& util_decodeToGraphNode( uint64_t pt, GridGraph& graph )
  {
    auto y = (uint32_t)pt;
    auto x = (uint32_t)( pt >> 32 );
    return graph.grid[x][y];
  }

  MapPath pathAStarSearch( GridGraph& graph, const MapPoint2& start, const MapPoint2& end )
  {
    Heap<uint64_t, Real> openTiles;
    std::map<uint64_t, uint64_t> parent;
    std::map<uint64_t, Real> gmap;

    openTiles.push( std::make_pair( util_encodePoint( start ), 0.0f ) );

    while ( !openTiles.empty() )
    {
      auto curPt = openTiles.top().first;
      auto& current = util_decodeToGraphNode( curPt, graph );
      if ( curPt == util_encodePoint( end ) )
      {
        MapPath reversePath;
        while ( curPt != parent[curPt] )
        {
          reversePath.push_back( util_decodeToGraphNode( curPt, graph ) );
          curPt = parent[curPt];
        }
        reversePath.push_back( start );
        std::reverse( reversePath.begin(), reversePath.end() );
        return reversePath;
      }

      openTiles.pop();
      current.closed = true;

      auto minx = std::max( current.x - 1, 0 );
      auto maxx = std::min( current.x + 1, graph.width - 1 );
      auto miny = std::max( current.y - 1, 0 );
      auto maxy = std::min( current.y + 1, graph.height - 1 );

      for ( auto x = minx; x <= maxx; ++x )
      {
        for ( auto y = miny; y <= maxy; ++y )
        {
          auto& neighbour = graph.grid[x][y];
          auto neighPt = util_encodePoint( neighbour );

          if ( !neighbour.valid || neighbour.closed )
            continue;

          auto g = gmap[curPt] + graph.cost( current, neighbour );
          auto h = util_diagonalDistanceHeuristic( neighbour, end );
          auto f = g + h;

          if ( gmap.find( neighPt ) == gmap.end() || gmap[neighPt] > g )
          {
            gmap[neighPt] = g;
            openTiles.set( neighPt, f );
            parent[neighPt] = curPt;
          }
        }
      }
    }

    return MapPath();
  }

}