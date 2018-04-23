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

#if HIVE_PATHING_USE_FIXED_POINT
    const PathCost DiagonalStepWeight = 1414; // sqrt(2) == diagonal multiplier
    const PathCost NormalStepWeight = 1000;
    const PathCost ObstacleStepWeight = 10000000;
    const PathCost c_inf = std::numeric_limits<int>::max();
    const PathCost c_dstarEpsilon = 0;
#else
    const PathCost DiagonalStepWeight = 1.41421f; // sqrt(2) == diagonal multiplier
    const PathCost NormalStepWeight = 1.0f;
    const PathCost ObstacleStepWeight = 10000.0f;
    const PathCost c_inf = std::numeric_limits<Real>::infinity();
    const PathCost c_dstarEpsilon = 0.01f;
#endif
    
    static bool isInfinity(PathCost cost)
    {
      return cost == c_inf;
    }

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

    PathCost GridGraph::cost( const GridGraphNode& from, const GridGraphNode& to ) const
    {
      if(to.hasObstacle || from.hasObstacle)
        return ObstacleStepWeight;

      if ( to.location.x != from.location.x && to.location.y != from.location.y )
        return DiagonalStepWeight;
      else
        return NormalStepWeight;
    }

    inline PathCost util_diagonalDistanceHeuristic( const MapPoint2& a, const MapPoint2& b )
    {
      auto dx = math::abs( b.x - a.x );
      auto dy = math::abs( b.y - a.y );
      return (NormalStepWeight * (dx + dy)) + ((DiagonalStepWeight - (2 * NormalStepWeight)) * std::min( dx, dy ));
    }

    inline PathCost util_chebyshevDistanceHeuristic( const MapPoint2& a, const MapPoint2& b )
    {
      auto dx = math::abs( b.x - a.x );
      auto dy = math::abs( b.y - a.y );
      return NormalStepWeight * std::max( dx, dy );
    }

    inline GridGraphNode& getNode( NodeIndex pt, GridGraph& graph )
    {
      return graph.grid[pt.x][pt.y];
    }

    inline bool dstarLess( DStarLiteKey a, DStarLiteKey b )
    {
      if(isInfinity(a.first))
        return false;

      DStarLiteKey aa = a;
      aa.first -= c_dstarEpsilon;
      aa.second -= c_dstarEpsilon;
      return aa < b;
    }

    inline bool dstarEquals( PathCost a, PathCost b )
    {
      if ( isInfinity( a ) && isInfinity( b ) )
        return true;
      return ( abs( a - b ) <= c_dstarEpsilon );
    }

    inline PathCost heuristic( const MapPoint2& a, const MapPoint2& b )
    {
      return util_diagonalDistanceHeuristic( a, b );
      //return util_chebyshevDistanceHeuristic(a, b);
    }

    DStarLiteKey DStarLite::calculateKey( NodeIndex sid, PathCost k_m )
    {
      auto& s = getNode( sid, *graph_ );
      auto& start = getNode( start_, *graph_ );

      auto minValue = std::min(s.g, s.rhs);

      if(isInfinity(minValue))
      {
        return DStarLiteKey{ c_inf, c_inf };
      }

      DStarLiteKey key(
        minValue + heuristic( s.location, start.location ) + k_m,
        minValue
      );

      return key;
    }

    void DStarLite::initialize( const MapPoint2& start, const MapPoint2& goal )
    {
      start_ = start;
      goal_ = goal;

      U.clear(); // U = null
      k_m = 0; // k_m = 0
      graph_->initialize(); // rhs(s) = g(s) = inf
      auto& goalNode = graph_->node(goal);
      goalNode.rhs = 0; // rhs(s_goal) = 0
      U.push( {&goalNode, calculateKey(goal_, k_m)} ); // U.Insert(s_goal, [h(s_start, s_goal); 0])
    }


    void DStarLite::updateVertex(NodeIndex uid)
    {
      if(uid != goal_)
      {
        auto& u = getNode(uid, *graph_);

        u.rhs = getNextValue(uid);
      }

      updateVertexLite(uid);
    }

    void DStarLite::updateVertexLite(NodeIndex uid)
    {
      auto& u = getNode(uid, *graph_);

      if(dstarEquals(u.g, u.rhs))
      {
        //graph_->console_->printf("u=(%d, %d) is already saturated with u.g=u.rhs=%f", uid.x, uid.y, u.g);
        //graph_->console_->printf("u=(%d, %d) is already saturated with u.g=u.rhs=%d", uid.x, uid.y, u.g);

        U.erase(&u);
      }
      else
      {
        auto key = calculateKey(uid, k_m);
        U.set(&u, key);
      }
    }

    static bool isValidNeighbour(NodeIndex neighbour, const GridGraph& graph)
    {
      if(neighbour.x < 0 || neighbour.x >= graph.width_)
        return false;
      if(neighbour.y < 0 || neighbour.y >= graph.height_)
        return false;

      return graph.node(neighbour).valid;
    }

    static const NodeIndex neighbourDeltas[] =
    {
      {-1,-1 },
      { 0,-1 },
      { 1,-1 },
      {-1, 0 },
      { 1, 0 },
      {-1, 1 },
      { 0, 1 },
      { 1, 1 },
    };

    static NodeIndex add(NodeIndex location, NodeIndex delta)
    {
      return { location.x + delta.x, location.y + delta.y };
    }

    void DStarLite::computeShortestPath()
    {
      auto& start = getNode(start_, *graph_);
      auto& goal = getNode(goal_, *graph_);

      int steps = 0;
      const int maxSteps = 100000;

      while(!U.empty() && steps++ < maxSteps)
      {
        auto topKey = U.topKey();
        auto startKey = calculateKey(start_, k_m);

        if(!dstarLess(topKey, startKey) && dstarEquals(start.rhs, start.g))
        {
          // Found the shortest path. The remaining elements in U are off-path.
          break;
        }

        auto& u = *U.top().first;

        //graph_->console_->printf("Node u=(%d, %d) visited with g(u) = %f, rhs(u) = %f, key={%f, %f}, h(u,start)=%f", u.location.x, u.location.y, u.g, u.rhs, topKey.first, topKey.second, heuristic( u.location, start.location ));
        //graph_->console_->printf("Node u=(%d, %d) visited with g(u) = %d, rhs(u) = %d, key={%d, %d}, h(u,start)=%d", u.location.x, u.location.y, u.g, u.rhs, topKey.first, topKey.second, heuristic( u.location, start.location ));

        // LPA*
#if 1
        {
          U.pop();

          if(u.rhs < u.g)
          {
            u.g = u.rhs;

            //graph_->console_->printf("u=(%d, %d) saturated with u.g=u.rhs=%f", u.location.x, u.location.y, u.g);
            //graph_->console_->printf("u=(%d, %d) saturated with u.g=u.rhs=%d", u.location.x, u.location.y, u.g);

            for(auto delta : neighbourDeltas)
            {
              MapPoint2 neighbour = add(u.location, delta);
              if(!isValidNeighbour(neighbour, *graph_))
                continue;

              if(neighbour == goal_)
                continue;

              auto& node = graph_->node(neighbour);
              auto newValue = u.g + graph_->cost(u, node);

              if(newValue < node.rhs)
              {
                node.rhs = newValue;

                //updateVertexLite(neighbour);
              }

              updateVertexLite(neighbour);
            }
          }
          else
          {
            u.g = c_inf;
            updateVertex(u.location);

            for(auto delta : neighbourDeltas)
            {
              MapPoint2 neighbour = add(u.location, delta);
              if(!isValidNeighbour(neighbour, *graph_))
                continue;

              updateVertex(neighbour);
            }
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
                auto min_rhs = std::numeric_limits<PathCost>::max();
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
        auto topKey = (!U.empty() ? U.topKey() : DStarLiteKey{ c_inf, c_inf });
        //graph_->console_->printf("Ending search with U.size=%d, U.topKey={%f,%f}, start.rhs=%f, start.g=%f, steps=%d", U.size(), topKey.first, topKey.second, start.rhs, start.g, steps );
        graph_->console_->printf("Ending search with U.size=%d, U.topKey={%d,%d}, start.rhs=%d, start.g=%d, steps=%d", U.size(), topKey.first, topKey.second, start.rhs, start.g, steps );
      }
    }

    pair<PathCost, NodeIndex> DStarLite::getNext(NodeIndex current) const
    {
      auto& node = graph_->node(current);

      PathCost bestValue = c_inf;
      MapPoint2 bestSucc;

      for(auto delta : neighbourDeltas)
      {
        MapPoint2 neighbour = add(node.location, delta);
        if(!isValidNeighbour(neighbour, *graph_))
          continue;

        auto& ss = getNode(neighbour, *graph_);

        if(isInfinity(ss.g))
          continue;

        auto value = ss.g + graph_->cost(node, ss);

        if(value < bestValue)
        {
          bestSucc = neighbour;
          bestValue = value;
        }
      }

      return { bestValue, bestSucc };
    }

    PathCost DStarLite::getNextValue(NodeIndex current) const
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

      if(v != goal_)
      {
        v.rhs = c_inf;
      }

      updateVertex(obstacle);

      for(auto delta : neighbourDeltas)
      {
        MapPoint2 neighbour = add(v.location, delta);
        if(!isValidNeighbour(neighbour, *graph_))
          continue;

        updateVertex(neighbour);
      }
    }
  }
}

