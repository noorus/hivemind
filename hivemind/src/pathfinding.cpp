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

    GridGraph::GridGraph( Bot* bot, Map& map ):
      map_( map ),
      bot_(bot),
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
      if ( !node.valid || map_.isBlocked( node.location.x, node.location.y ) )
        return false;

      return true;
    }

    Real GridGraph::cost( const GridGraphNode& from, const GridGraphNode& to ) const
    {
      Real weight = 1.0f;

      if(to.hasObstacle)
        return 10000.0f;

      if ( to.location.x != from.location.x && to.location.y != from.location.y )
        return (weight * c_sqrt2f);
      else
        return weight;
    }

    inline Real util_diagonalDistanceHeuristic( const MapPoint2& a, const MapPoint2& b )
    {
      auto dx = (Real)math::abs( b.x - a.x );
      auto dy = (Real)math::abs( b.y - a.y );
      Real D = 1.0f;
      return (D * (dx + dy)) + ((c_sqrt2f - (2.0f * D)) * std::min( dx, dy ));
    }

    inline Real util_chebyshevDistanceHeuristic( const MapPoint2& a, const MapPoint2& b )
    {
      auto dx = (Real)math::abs( b.x - a.x );
      auto dy = (Real)math::abs( b.y - a.y );
      return std::max( dx, dy );
    }

    inline GridGraphNode& getNode( NodeIndex pt, GridGraph& graph )
    {
      return graph.grid[pt.x][pt.y];
    }

    inline bool dstarEquals( Real a, Real b )
    {
      if ( isinf( a ) && isinf( b ) )
        return true;
      return ( abs( a - b ) < c_dstarEpsilon );
    }

    inline Real heuristic( const MapPoint2& a, const MapPoint2& b )
    {
      // return util_diagonalDistanceHeuristic( a, b );
      return util_chebyshevDistanceHeuristic(a, b);
    }

    DStarLiteKey DStarLite::calculateKey( NodeIndex sid, Real k_m )
    {
      auto& s = getNode( sid, graph_ );
      auto& start = getNode( start_, graph_ );

      DStarLiteKey key(
        ( std::min( s.g, s.rhs ) + heuristic( start.location, s.location ) + k_m ),
        ( std::min( s.g, s.rhs ) )
      );

      return key;
    }

    void DStarLite::initialize( const MapPoint2& start, const MapPoint2& goal )
    {
      start_ = start;
      goal_ = goal;

      U.clear(); // U = null
      k_m = 0.0f; // k_m = 0
      graph_.process( c_inf, c_inf ); // rhs(s) = g(s) = inf
      graph_.node( goal ).rhs = 0.0f; // rhs(s_goal) = 0
      U.push( std::make_pair( goal_, calculateKey(goal_, 0.0f) ) ); // U.Insert(s_goal, [h(s_start, s_goal); 0])
    }

    void DStarLite::updateVertex(NodeIndex uid)
    {
      if(uid == goal_)
        return;

      auto& u = getNode(uid, graph_);
      u.rhs = getNextValue(uid);

      if(dstarEquals(u.g, u.rhs))
      {
        U.erase(uid);
      }
      else
      {
        // set does Update or Insert accordingly.
        U.set(uid, calculateKey(uid, k_m));
      }
    }

    // Predecessors are all neighbors that are walkable.
    vector<NodeIndex> DStarLite::predecessors( GridGraphNode& s ) const
    {
      vector<NodeIndex> vec;
      auto minx = std::max( s.location.x - 1, 0 );
      auto maxx = std::min( s.location.x + 1, graph_.width_ - 1 );
      auto miny = std::max( s.location.y - 1, 0 );
      auto maxy = std::min( s.location.y + 1, graph_.height_ - 1 );
      for ( auto x = minx; x <= maxx; ++x )
      {
        for ( auto y = miny; y <= maxy; ++y )
        {
          MapPoint2 neighbour = { x, y };
          if ( s.location == neighbour )
            continue;
          if ( graph_.node( neighbour ).valid )
            vec.push_back( neighbour );
        }
      }
      return vec;
    }

    vector<NodeIndex> DStarLite::successors( GridGraphNode& s ) const
    {
      return predecessors(s);
    }

    void DStarLite::computeShortestPath()
    {
      auto& start = getNode( start_, graph_ );
      auto& goal = getNode( goal_, graph_ );

      while(!U.empty() && (U.topKey() < calculateKey(start_, k_m) || !dstarEquals(start.rhs, start.g)))
      {
        auto& uv = U.top();
        auto& u = getNode(uv.first, graph_);

        graph_.bot_->console().printf("Node u=(%d, %d) visited with g(u) = %f, rhs(u) = %f", u.location.x, u.location.y, u.g, u.rhs);

        // LPA*
#if 1
        {
          U.erase(uv.first);
          if(u.g > u.rhs)
          {
            u.g = u.rhs;
          }
          else
          {
            u.g = c_inf;
            updateVertex(uv.first);
          }

          for(auto s : successors(u))
          {
            updateVertex(s);
          }
        }
#else
        // D*-lite
        {
          auto& k_old = uv.second;
          auto k_new = calculateKey(uv.first, k_m);

          if(k_old < k_new)
          {
            graph_.bot_->console().printf("Node u=(%d, %d) visited with k_old = %f < k_new = %f", u.location.x, u.location.y, k_old, k_new);

            U.set(uv.first, k_new);
          }
          else if ( u.g > u.rhs )
          {
            u.g = u.rhs;

            graph_.bot_->console().printf("Node u=(%d, %d) visited with g(u) = %f", u.location.x, u.location.y, u.g);

            U.erase( uv.first );
            auto preds = predecessors( u );
            for ( auto sid : preds )
            {
              if ( sid != goal_ )
              {
                auto& s = getNode( sid, graph_ );
                s.rhs = std::min( s.rhs, graph_.cost( s, u ) + u.g );
              }
              updateVertex( sid );
            }
          }
          else
          {
            auto g_old = u.g;
            u.g = c_inf;

            graph_.bot_->console().printf("Node u=(%d, %d) expanded with g(u)=%f", u.location.x, u.location.y, g_old);

            auto preds = predecessors( u );
            preds.push_back( uv.first ); // Pred(u) union {u}
            for ( auto sid : preds )
            {
              auto& s = getNode( sid, graph_ );
              if ( dstarEquals( s.rhs, graph_.cost( s, u ) + g_old ) && sid != goal_ )
              {
                auto min_rhs = std::numeric_limits<Real>::max();
                auto succs = successors( s );
                for ( auto succid : succs )
                {
                  auto& subsucc = getNode( succid, graph_ );
                  min_rhs = std::min( min_rhs, subsucc.g + graph_.cost( s, subsucc ) );
                }
              }
              updateVertex( sid );
            }
          }
        }
#endif
      }
    }

    pair<Real, NodeIndex> DStarLite::getNext(NodeIndex current) const
    {
      auto& node = graph_.node(current);

      Real bestValue = c_inf;
      MapPoint2 bestSucc;

      for(auto& s : successors(node))
      {
        auto& ss = getNode(s, graph_);

        auto value = ss.g + graph_.cost(node, ss);

        if(value < bestValue)
        {
          bestSucc = s;
          bestValue = value;
        }
      }

      return { bestValue, bestSucc };
    }

    Real DStarLite::getNextValue(NodeIndex current) const
    {
      return getNext(current).first;
    }

    NodeIndex DStarLite::getNextNode(NodeIndex current) const
    {
      return getNext(current).second;
    }

    MapPath DStarLite::getMapPath() const
    {
      MapPath mapPath;

      auto startNode = graph_.node(start_);
      if(startNode.rhs < c_inf)
      {
        auto s = start_;
        while(s != goal_)
        {
          mapPath.push_back(s);
          s = getNextNode(s);
        }
        mapPath.push_back(goal_);
      }

      return mapPath;
    }

    DStarLite::DStarLite(GridGraph& graph, const MapPoint2& start, const MapPoint2& goal) :
      graph_(graph)
    {
      initialize(start, goal);
      computeShortestPath();
    }

    DStarLitePtr DStarLite::search(GridGraph& graph, const MapPoint2& start, const MapPoint2& goal)
    {
      return DStarLitePtr(new DStarLite(graph, start, goal));
    }

    void DStarLite::update(MapPoint2 obstacle)
    {
      auto& v = graph_.node(obstacle);
      v.hasObstacle = true;
      v.g = c_inf;
      v.rhs = c_inf;

      updateVertex(obstacle);

      for(auto uu : predecessors(v))
      {
        updateVertex(uu);
      }
      computeShortestPath();
    }

    // basic A* implementation below
    MapPath pathAStarSearch( GridGraph& graph, const MapPoint2& start, const MapPoint2& goal )
    {
      Heap<NodeIndex, Real> openTiles;
      std::map<NodeIndex, NodeIndex> parent;
      std::map<NodeIndex, Real> gmap;

      openTiles.push( std::make_pair( start , 0.0f ) );

      NodeIndex curPt;

      while ( !openTiles.empty() )
      {
        curPt = openTiles.top().first;
        auto& current = getNode( curPt, graph );
        if ( curPt == goal )
        {
          goto returnCurrent;
        }

        openTiles.pop();
        current.closed = true;

        auto minx = std::max( current.location.x - 1, 0 );
        auto maxx = std::min( current.location.x + 1, graph.width_ - 1 );
        auto miny = std::max( current.location.y - 1, 0 );
        auto maxy = std::min( current.location.y + 1, graph.height_ - 1 );

        for ( auto x = minx; x <= maxx; ++x )
        {
          for ( auto y = miny; y <= maxy; ++y )
          {
            auto& neighbour = graph.grid[x][y];
            auto neighPt = neighbour;

            if ( !neighbour.valid || neighbour.closed || !graph.valid( neighbour ) )
              continue;

            auto g = gmap[curPt] + graph.cost( current, neighbour );
            auto h = util_diagonalDistanceHeuristic( neighbour.location, goal );
            auto f = g + h;

            if ( gmap.find( neighPt.location ) == gmap.end() || gmap[neighPt.location] > g )
            {
              gmap[neighPt.location] = g;
              openTiles.set( neighPt.location, f );
              parent[neighPt.location] = curPt;
            }
          }
        }
      }

      // unwind from current position to start and return path, regardless of whether we found the goal
      returnCurrent:

      MapPath reversePath;
      while ( curPt != parent[curPt] )
      {
        reversePath.push_back( curPt );
        curPt = parent[curPt];
      }
      reversePath.push_back( start );
      std::reverse( reversePath.begin(), reversePath.end() );
      return reversePath;
    }

  }

}