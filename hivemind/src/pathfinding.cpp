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

  Real util_diagonalDistanceHeuristic( const GridGraphNode& a, const MapPoint2& b )
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
      auto pid = openTiles.top().first;
      auto& p = util_decodeToGraphNode( pid, graph );
      if ( pid == util_encodePoint( end ) )
      {
        MapPath reversePath;
        while ( pid != parent[pid] )
        {
          reversePath.push_back( util_decodeToGraphNode( pid, graph ) );
          pid = parent[pid];
        }
        reversePath.push_back( start );
        std::reverse( reversePath.begin(), reversePath.end() );
        return reversePath;
      }

      //auto fvalue = openTiles.top().second;
      //auto gvalue = gmap[pid];

      openTiles.pop();
      p.closed = true;

      auto minx = std::max( p.x - 1, 0 );
      auto maxx = std::min( p.x + 1, graph.width - 1 );
      auto miny = std::max( p.y - 1, 0 );
      auto maxy = std::min( p.y + 1, graph.height - 1 );

      for ( auto x = minx; x <= maxx; ++x )
      {
        for ( auto y = miny; y <= maxy; ++y )
        {
          auto& current = graph.grid[x][y];
          auto idCurrent = util_encodePoint( current );

          if ( !current.valid || current.closed )
            continue;

          auto g = gmap[pid] + current.cost( p );
          auto h = util_diagonalDistanceHeuristic( current, end );
          auto f = g + h;

          if ( gmap.find( idCurrent ) == gmap.end() || gmap[idCurrent] > g )
          {
            gmap[idCurrent] = g;
            openTiles.set( idCurrent, f );
            parent[idCurrent] = pid;
          }
        }
      }
    }

    return MapPath();
  }

}