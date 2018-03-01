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
    const Real c_dstarEpsilon = 0.01f;

    GridGraph::GridGraph( Console* console,  std::unique_ptr<GridMap> map ):
      map_( std::move(map) ),
      console_(console),
      width_( map_->width() ),
      height_( map_->height() )
    {
      grid.resize( width_, height_ );
    }

    void GridGraph::initialize()
    {
      for ( int x = 0; x < width_; ++x )
        for ( int y = 0; y < height_; ++y )
        {
          GridGraphNode node( x, y );
          node.valid = map_->isWalkable(x, y);
          node.rhs = c_inf;
          node.g = c_inf;
          grid[x][y] = node;
        }
    }

    Real GridGraph::cost( const GridGraphNode& from, const GridGraphNode& to ) const
    {
      Real weight = 1.0f;

      if(to.hasObstacle || from.hasObstacle)
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

    inline bool dstarLess( DStarLiteKey a, DStarLiteKey b )
    {
      DStarLiteKey aa = a;
      aa.first -= c_dstarEpsilon;
      aa.second -= c_dstarEpsilon;
      return aa < b;
    }

    inline bool dstarEquals( Real a, Real b )
    {
      if ( isinf( a ) && isinf( b ) )
        return true;
      return ( abs( a - b ) < c_dstarEpsilon );
    }

    inline Real heuristic( const MapPoint2& a, const MapPoint2& b )
    {
      return util_diagonalDistanceHeuristic( a, b );
      //return util_chebyshevDistanceHeuristic(a, b);
    }

    DStarLiteKey DStarLite::calculateKey( NodeIndex sid, Real k_m )
    {
      auto& s = getNode( sid, *graph_ );
      auto& start = getNode( start_, *graph_ );

      DStarLiteKey key(
        ( std::min( s.g, s.rhs ) + heuristic( s.location, start.location ) + k_m ),
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
      graph_->initialize(); // rhs(s) = g(s) = inf
      graph_->node( goal ).rhs = 0.0f; // rhs(s_goal) = 0
      U.push( {goal_, calculateKey(goal_, k_m)} ); // U.Insert(s_goal, [h(s_start, s_goal); 0])
    }

    void DStarLite::updateVertex(NodeIndex uid)
    {
      auto& u = getNode(uid, *graph_);

      if(uid != goal_)
      {
        u.rhs = getNextValue(uid);
      }

      if(dstarEquals(u.g, u.rhs))
      {
        //graph_->console_->printf("u=(%d, %d) saturated with u.g=u.rhs=%f", uid.x, uid.y, u.g);

        U.erase(uid);
      }
      else
      {
        auto key = calculateKey(uid, k_m);
        U.set(uid, key);
      }
    }

    // Predecessors are all neighbors that are walkable.
    vector<NodeIndex> DStarLite::predecessors( const GridGraphNode& s ) const
    {
      vector<NodeIndex> vec;
      auto minx = std::max( s.location.x - 1, 0 );
      auto maxx = std::min( s.location.x + 1, graph_->width_ - 1 );
      auto miny = std::max( s.location.y - 1, 0 );
      auto maxy = std::min( s.location.y + 1, graph_->height_ - 1 );
      for ( auto x = minx; x <= maxx; ++x )
      {
        for ( auto y = miny; y <= maxy; ++y )
        {
          MapPoint2 neighbour = { x, y };
          if ( s.location == neighbour )
            continue;
          if ( graph_->node( neighbour ).valid )
            vec.push_back( neighbour );
        }
      }
      return vec;
    }

    vector<NodeIndex> DStarLite::successors( const GridGraphNode& s ) const
    {
      return predecessors(s);
    }

    void DStarLite::computeShortestPath()
    {
      auto& start = getNode(start_, *graph_);
      auto& goal = getNode(goal_, *graph_);

      while(!U.empty())
      {
        auto topKey = U.topKey();

        if(!dstarLess(topKey, calculateKey(start_, k_m)) && dstarEquals(start.rhs, start.g))
        {
          // Found the shortest path. The remaining elements in U are off-path.
          break;
        }

        auto& uv = U.top();
        auto& u = getNode(uv.first, *graph_);

        //graph_->console_->printf("Node u=(%d, %d) visited with g(u) = %f, rhs(u) = %f, key={%f, %f}, h(u,start)=%f", u.location.x, u.location.y, u.g, u.rhs, topKey.first, topKey.second, heuristic( u.location, start.location ));

        // LPA*
#if 1
        {
          U.pop();

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
          else if(u.g > u.rhs)
          {
            u.g = u.rhs;

            graph_.bot_->console().printf("Node u=(%d, %d) visited with g(u) = %f", u.location.x, u.location.y, u.g);

            U.erase(uv.first);
            auto preds = predecessors(u);
            for(auto sid : preds)
            {
              if(sid != goal_)
              {
                auto& s = getNode(sid, graph_);
                s.rhs = std::min(s.rhs, graph_.cost(s, u) + u.g);
              }
              updateVertex(sid);
            }
          }
          else
          {
            auto g_old = u.g;
            u.g = c_inf;

            graph_.bot_->console().printf("Node u=(%d, %d) expanded with g(u)=%f", u.location.x, u.location.y, g_old);

            auto preds = predecessors(u);
            preds.push_back(uv.first); // Pred(u) union {u}
            for(auto sid : preds)
            {
              auto& s = getNode(sid, graph_);
              if(dstarEquals(s.rhs, graph_.cost(s, u) + g_old) && sid != goal_)
              {
                auto min_rhs = std::numeric_limits<Real>::max();
                auto succs = successors(s);
                for(auto succid : succs)
                {
                  auto& subsucc = getNode(succid, graph_);
                  min_rhs = std::min(min_rhs, subsucc.g + graph_.cost(s, subsucc));
                }
              }
              updateVertex(sid);
            }
          }
        }
#endif
      }

      {
        //auto topKey = (!U.empty() ? U.topKey() : DStarLiteKey{ c_inf, c_inf });
        //graph_->console_->printf("Ending search with U.size=%d, U.topKey={%f,%f}, start.rhs=%f, start.g=%f", U.size(), topKey.first, topKey.second, start.rhs, start.g );
      }
    }

    pair<Real, NodeIndex> DStarLite::getNext(NodeIndex current) const
    {
      auto& node = graph_->node(current);

      Real bestValue = c_inf;
      MapPoint2 bestSucc;

      for(auto& s : successors(node))
      {
        auto& ss = getNode(s, *graph_);

        auto value = ss.g + graph_->cost(node, ss);

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

      auto startNode = graph_->node(start_);
      if(startNode.rhs < c_inf)
      {
        auto s = start_;
        while(s != goal_)
        {
          mapPath.push_back(s);
          s = getNextNode(s);

          if(mapPath.size() > 1000)
          {
            graph_->console_->printf("BUG: failed get path from (%d, %d) to (%d, %d)", start_.x, start_.y, goal_.x, goal_.y);
            break;
          }
        }
        mapPath.push_back(goal_);
      }

      return mapPath;
    }

    DStarLite::DStarLite(Console* console, std::unique_ptr<GridMap> map, const MapPoint2& start, const MapPoint2& goal)
    {
      graph_ = std::make_unique<pathfinding::GridGraph>( console, std::move(map) );
      initialize(start, goal);
      computeShortestPath();
    }

    DStarLitePtr DStarLite::search(Console* console, std::unique_ptr<GridMap> map, const MapPoint2& start, const MapPoint2& goal)
    {
      return DStarLitePtr(new DStarLite(console, std::move(map), start, goal));
    }

    void DStarLite::updateWalkability(MapPoint2 obstacle, bool hasObstacle)
    {
      auto& v = graph_->node(obstacle);
      v.hasObstacle = hasObstacle;
      v.g = c_inf;
      v.rhs = c_inf;

      updateVertex(obstacle);

      for(auto uu : predecessors(v))
      {
        updateVertex(uu);
      }
      computeShortestPath();
    }


  }
}

