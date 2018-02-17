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

namespace hivemind {

  namespace pathfinding {

    const Real c_sqrt2f = 1.41421f; // sqrt(2) == diagonal multiplier
    const Real c_inf = std::numeric_limits<Real>::infinity();

    GridGraph::GridGraph( Map& map ):
      map_( map ),
      width_( (int)map.width() ),
      height_( (int)map.height() )
    {
      grid.resize( width_, height_ );
    }

    void GridGraph::process( Real initial_g, Real initial_rhs )
    {
      for ( int x = 0; x < width_; ++x )
        for ( int y = 0; y < height_; ++y )
        {
          GridGraphNode node( x, y );
          node.valid = ( map_.flagsMap_[x][y] & MapFlag_Walkable );
          node.closed = false;
          node.rhs = initial_rhs;
          node.g = initial_g;
          grid[x][y] = node;
        }
    }

    void GridGraph::reset()
    {
      for ( int x = 0; x < width_; ++x )
        for ( int y = 0; y < height_; ++y )
        {
          grid[x][y].reset();
        }
    }

    bool GridGraph::valid( const GridGraphNode& node ) const
    {
      if ( !node.valid || map_.isBlocked( node.x, node.y ) )
        return false;

      return true;
    }

    Real GridGraph::cost( const GridGraphNode& from, const GridGraphNode& to ) const
    {
      Real weight = 1.0f;

      if ( to.x != from.x && to.y != from.y )
        return (weight * c_sqrt2f);
      else
        return weight;
    }

    inline uint64_t util_encodePoint( const Point2DI& pt )
    {
      return (((uint64_t)pt.x) << 32) | ((uint64_t)pt.y);
    };

    inline uint64_t util_encodePoint( int x, int y )
    {
      return ( ( (uint64_t)x ) << 32 ) | ( (uint64_t)y );
    };

    inline Real util_diagonalDistanceHeuristic( const MapPoint2& a, const MapPoint2& b )
    {
      auto dx = (Real)math::abs( b.x - a.x );
      auto dy = (Real)math::abs( b.y - a.y );
      Real D = 1.0f;
      return (D * (dx + dy)) + ((c_sqrt2f - (2.0f * D)) * std::min( dx, dy ));
    }

    inline GridGraphNode& util_decodeToGraphNode( uint64_t pt, GridGraph& graph )
    {
      auto y = (uint32_t)pt;
      auto x = (uint32_t)(pt >> 32);
      return graph.grid[x][y];
    }

    // D* lite implementation

    inline bool dstarEquals( Real a, Real b )
    {
      if ( isinf( a ) && isinf( b ) )
        return true;
      return ( abs( a - b ) < c_dstarEpsilon );
    }

    inline Real heuristic( const MapPoint2& a, const MapPoint2& b )
    {
      // ??
      // return util_diagonalDistanceHeuristic( a, b );
      // ??
      auto dx = (Real)math::abs( b.x - a.x );
      auto dy = (Real)math::abs( b.y - a.y );
      return std::max( dx, dy );
    }

    DStarLiteKey DStarLite::calculateKey( uint64_t sid, Real km )
    {
      auto& s = util_decodeToGraphNode( sid, graph_ );
      auto& start = util_decodeToGraphNode( start_, graph_ );

      DStarLiteKey key(
        ( std::min( s.g, s.rhs ) + heuristic( start, s ) + km ),
        ( std::min( s.g, s.rhs ) )
      );

      return key;
    }

    void DStarLite::initialize( const MapPoint2& from, const MapPoint2& goal )
    {
      start_ = util_encodePoint( from );
      goal_ = util_encodePoint( goal );

      U.clear(); // U = null
      k_m = 0.0f; // k_m = 0
      graph_.process( c_inf, c_inf ); // rhs(s) = g(s) = inf
      graph_.node( goal ).rhs = 0.0f; // rhs(s_goal) = 0
      U.push( std::make_pair( util_encodePoint( from ), DStarLiteKey( heuristic( from, goal ), 0.0f ) ) ); // U.Insert(s_goal, [h(s_start, s_goal); 0])
    }

    void DStarLite::updateVertex( uint64_t uid )
    {
      auto& u = util_decodeToGraphNode( uid, graph_ );

      if ( !dstarEquals( u.g, u.rhs ) )
      {
        // set does Update or Insert accordingly
        U.set( uid, calculateKey( uid, k_m ) );
      } else
        U.erase( uid );
    }

    // TODO ensure validity of this -
    // predecessors are all neighbors that are walkable
    vector<uint64_t> DStarLite::predecessors( GridGraphNode& s )
    {
      vector<uint64_t> vec;
      auto minx = std::max( s.x - 1, 0 );
      auto maxx = std::min( s.x + 1, graph_.width_ - 1 );
      auto miny = std::max( s.y - 1, 0 );
      auto maxy = std::min( s.y + 1, graph_.height_ - 1 );
      for ( auto x = minx; x <= maxx; ++x )
      {
        for ( auto y = miny; y <= maxy; ++y )
        {
          if ( s.x == x && s.y == y )
            continue;
          if ( graph_.node( x, y ).valid )
            vec.push_back( util_encodePoint( x, y ) );
        }
      }
      return vec;
    }

    // TODO ensure validity of this -
    // successors are all neighbors regardless of walkability,
    // but empty if current node is not walkable
    vector<uint64_t> DStarLite::successors( GridGraphNode& s )
    {
      vector<uint64_t> vec;
      if ( !s.valid )
        return vec;

      auto minx = std::max( s.x - 1, 0 );
      auto maxx = std::min( s.x + 1, graph_.width_ - 1 );
      auto miny = std::max( s.y - 1, 0 );
      auto maxy = std::min( s.y + 1, graph_.height_ - 1 );
      for ( auto x = minx; x <= maxx; ++x )
      {
        for ( auto y = miny; y <= maxy; ++y )
        {
          if ( s.x == x && s.y == y )
            continue;
          vec.push_back( util_encodePoint( x, y ) );
        }
      }
      return vec;
    }

    void DStarLite::computeShortestPath()
    {
      auto& start = util_decodeToGraphNode( start_, graph_ );
      auto& goal = util_decodeToGraphNode( goal_, graph_ );

      while ( U.topKey() < calculateKey( start_, k_m ) || start.rhs > start.g )
      {
        auto& uv = U.top();
        auto& u = util_decodeToGraphNode( uv.first, graph_ );

        auto& k_old = uv.second;
        auto k_new = calculateKey( uv.first, k_m );

        if ( k_old < k_new )
          U.set( uv.first, k_new );
        else if ( u.g > u.rhs )
        {
          u.g = u.rhs;
          U.erase( uv.first );
          auto preds = predecessors( u );
          for ( auto sid : preds )
          {
            if ( sid != goal_ )
            {
              auto& s = util_decodeToGraphNode( sid, graph_ );
              s.rhs = std::min( s.rhs, graph_.cost( s, u ) + s.g );
            }
            updateVertex( sid );
          }
        }
        else
        {
          auto g_old = u.g;
          u.g = c_inf;
          auto preds = predecessors( u );
          preds.push_back( uv.first ); // Pred(u) union {u}
          for ( auto sid : preds )
          {
            auto& s = util_decodeToGraphNode( sid, graph_ );
            if ( dstarEquals( s.rhs, graph_.cost( s, u ) + g_old ) && sid != goal_ )
            {
              auto min_rhs = std::numeric_limits<Real>::max();
              auto succs = successors( s );
              for ( auto succid : succs )
              {
                auto& subsucc = util_decodeToGraphNode( succid, graph_ );
                min_rhs = std::min( min_rhs, subsucc.g + graph_.cost( s, subsucc ) );
              }
            }
            updateVertex( sid );
          }
        }
      }
    }

    // basic A* implementation below

    MapPath pathAStarSearch( GridGraph& graph, const MapPoint2& start, const MapPoint2& end )
    {
      Heap<uint64_t, Real> openTiles;
      std::map<uint64_t, uint64_t> parent;
      std::map<uint64_t, Real> gmap;

      openTiles.push( std::make_pair( util_encodePoint( start ), 0.0f ) );

      uint64_t curPt = 0;

      while ( !openTiles.empty() )
      {
        curPt = openTiles.top().first;
        auto& current = util_decodeToGraphNode( curPt, graph );
        if ( curPt == util_encodePoint( end ) )
        {
          goto returnCurrent;
        }

        openTiles.pop();
        current.closed = true;

        auto minx = std::max( current.x - 1, 0 );
        auto maxx = std::min( current.x + 1, graph.width_ - 1 );
        auto miny = std::max( current.y - 1, 0 );
        auto maxy = std::min( current.y + 1, graph.height_ - 1 );

        for ( auto x = minx; x <= maxx; ++x )
        {
          for ( auto y = miny; y <= maxy; ++y )
          {
            auto& neighbour = graph.grid[x][y];
            auto neighPt = util_encodePoint( neighbour );

            if ( !neighbour.valid || neighbour.closed || !graph.valid( neighbour ) )
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

      // unwind from current position to start and return path, regardless of whether we found the goal
      returnCurrent:

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

  }

}