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

  namespace pathfinding {

    const Real c_sqrt2f = 1.41421f; // sqrt(2) == diagonal multiplier

    GridGraph::GridGraph( Map& map ):
      map_( map ),
      width_( (int)map.width() ),
      height_( (int)map.height() )
    {
      grid.resize( width_, height_ );
      for ( int x = 0; x < width_; ++x )
        for ( int y = 0; y < height_; ++y )
        {
          GridGraphNode node( x, y );
          node.valid = (map_.flagsMap_[x][y] & MapFlag_Walkable);
          node.closed = false;
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

    void GridGraph::applyTo( Dstar * dstar )
    {
      for ( int x = 0; x < width_; ++x )
        for ( int y = 0; y < height_; ++y )
        {
          dstar->updateCell( x, y, grid[x][y].valid ? 1.0 : -1.0 );
        }
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

      return MapPath();
    }



    // D* lite ========================================================================================================



    const double C1 = 1.0; // cost of an unseen cell

    constexpr float keyHashCode( DStarState u )
    {
      return (float)( u.k.first + 1193 * u.k.second );
    }

    bool Dstar::isValid( DStarState u )
    {
      auto it = openHash.find( u );
      if ( it == openHash.end() )
        return false;
      if ( !math::equals( keyHashCode( u ), it->second ) )
        return false;
      return true;
    }

    bool Dstar::occupied( DStarState u )
    {
      auto it = cellHash.find( u );
      if ( it == cellHash.end() )
        return false;
      return (it->second.cost < 0);
    }

    void Dstar::init( int sX, int sY, int gX, int gY )
    {
      cellHash.clear();
      path.clear();
      openHash.clear();
      while ( !openList.empty() )
        openList.pop();

      k_m = 0;

      s_start.x = sX;
      s_start.y = sY;
      s_goal.x = gX;
      s_goal.y = gY;

      DStarCell tmp;
      tmp.g = tmp.rhs = 0;
      tmp.cost = C1;

      cellHash[s_goal] = tmp;

      tmp.g = tmp.rhs = heuristic( s_start, s_goal );
      tmp.cost = C1;
      cellHash[s_start] = tmp;
      s_start = calculateKey( s_start );

      s_last = s_start;
    }

    void Dstar::makeNewCell( DStarState u )
    {
      if ( cellHash.find( u ) != cellHash.end() )
        return;

      DStarCell tmp;
      tmp.g = tmp.rhs = heuristic( u, s_goal );
      tmp.cost = C1;
      cellHash[u] = tmp;
    }

    double Dstar::getG( DStarState u )
    {
      if ( cellHash.find( u ) == cellHash.end() )
        return heuristic( u, s_goal );
      return cellHash[u].g;
    }

    double Dstar::getRHS( DStarState u )
    {
      if ( u == s_goal )
        return 0;

      if ( cellHash.find( u ) == cellHash.end() )
        return heuristic( u, s_goal );
      return cellHash[u].rhs;
    }

    void Dstar::setG( DStarState u, double g )
    {
      makeNewCell( u );
      cellHash[u].g = g;
    }

    double Dstar::setRHS( DStarState u, double rhs )
    {
      makeNewCell( u );
      cellHash[u].rhs = rhs;
      return rhs;
    }

    /* int Dstar::computeShortestPath()
    * --------------------------
    * As per [S. Koenig, 2002] except for 2 main modifications:
    * 1. We stop planning after a number of steps, 'maxsteps' we do this
    *    because this algorithm can plan forever if the start is
    *    surrounded by obstacles.
    * 2. We lazily remove states from the open list so we never have to
    *    iterate through it.
    */
    int Dstar::computeShortestPath( int maxSteps )
    {
      list<DStarState> s;

      if ( openList.empty() )
        return 1;

      int k = 0;
      while ( (!openList.empty()) &&
        (openList.top() < (s_start = calculateKey( s_start ))) ||
        (getRHS( s_start ) != getG( s_start )) )
      {

        if ( k++ > maxSteps )
        {
          fprintf( stderr, "At maxsteps\n" );
          return -1;
        }


        DStarState u;

        bool test = (getRHS( s_start ) != getG( s_start ));

        // lazy remove
        while ( true )
        {
          if ( openList.empty() ) return 1;
          u = openList.top();
          openList.pop();

          if ( !isValid( u ) ) continue;
          if ( !(u < s_start) && (!test) ) return 2;
          break;
        }

        DStarOpenHashMap::iterator cur = openHash.find( u );
        openHash.erase( cur );

        DStarState k_old = u;

        if ( k_old < calculateKey( u ) )
        {
          // u is out of date
          insert( u );
        }
        else if ( getG( u ) > getRHS( u ) )
        {
          // needs update (got better)
          setG( u, getRHS( u ) );
          getPredecessors( u, s );
          for (auto & i : s)
          {
            updateVertex( i );
          }
        }
        else
        {
          // g <= rhs, state has got worse
          setG( u, INFINITY );
          getPredecessors( u, s );
          for (auto & i : s)
          {
            updateVertex( i );
          }
          updateVertex( u );
        }
      }
      return 0;
    }

    void Dstar::updateVertex( DStarState u )
    {
      list<DStarState> s;

      if ( u != s_goal )
      {
        getSuccessors( u, s );
        double tmp = INFINITY;
        double tmp2;

        for (auto & i : s)
        {
          tmp2 = getG( i ) + cost( u, i );
          if ( tmp2 < tmp ) tmp = tmp2;
        }
        if ( !math::equals( getRHS( u ), tmp ) )
          setRHS( u, tmp );
      }

      if ( !math::equals( getG( u ), getRHS( u ) ) )
        insert( u );

    }

    void Dstar::insert( DStarState u )
    {

      DStarOpenHashMap::iterator cur;
      float csum;

      u = calculateKey( u );
      cur = openHash.find( u );
      csum = keyHashCode( u );
      // return if cell is already in list. TODO: this should be
      // uncommented except it introduces a bug, I suspect that there is a
      // bug somewhere else and having duplicates in the openList queue
      // hides the problem...
      //if ((cur != openHash.end()) && (close(csum,cur->second))) return;

      openHash[u] = csum;
      openList.push( u );
    }

    void Dstar::remove( DStarState u )
    {
      auto cur = openHash.find( u );
      if ( cur == openHash.end() )
        return;
      openHash.erase( cur );
    }

    double trueDist( const DStarState& a, const DStarState& b )
    {
      float x = a.x - b.x;
      float y = a.y - b.y;
      return sqrt( x*x + y * y );
    }

    // 8-way distance scaled by C1 (must be <= mincost)
    double Dstar::heuristic( DStarState a, DStarState b )
    {
      return (double)util_diagonalDistanceHeuristic( a, b );
      /*double min = fabs( a.x - b.x );
      double max = fabs( a.y - b.y );
      if ( min > max )
      {
        double temp = min;
        min = max;
        max = temp;
      }
      auto diff = ( ( M_SQRT2 - 1.0 )*min + max );
      return diff *C1;*/
    }

    DStarState Dstar::calculateKey( DStarState u )
    {
      double val = fmin( getRHS( u ), getG( u ) );

      u.k.first = val + heuristic( u, s_start ) + k_m;
      u.k.second = val;

      return u;
    }

    /* double Dstar::cost(state a, state b)
    * --------------------------
    * Returns the cost of moving from state a to state b. This could be
    * either the cost of moving off state a or onto state b, we went with
    * the former. This is also the 8-way cost.
    */
    double Dstar::cost( DStarState a, DStarState b )
    {
      int xd = fabs( a.x - b.x );
      int yd = fabs( a.y - b.y );
      double scale = 1;

      if ( xd + yd>1 )
        scale = M_SQRT2;

      if ( cellHash.count( a ) == 0 )
        return scale * C1;

      return scale * cellHash[a].cost;
    }

    void Dstar::updateCell( int x, int y, double val )
    {
      DStarState u;

      u.x = x;
      u.y = y;

      if ( u == s_start || u == s_goal )
        return;

      makeNewCell( u );
      cellHash[u].cost = val;

      updateVertex( u );
    }

    void Dstar::getSuccessors( DStarState u, list<DStarState> &s )
    {
      s.clear();
      u.k.first = -1;
      u.k.second = -1;

      if ( occupied( u ) )
        return;

      u.x += 1;
      s.push_front( u );
      u.y += 1;
      s.push_front( u );
      u.x -= 1;
      s.push_front( u );
      u.x -= 1;
      s.push_front( u );
      u.y -= 1;
      s.push_front( u );
      u.y -= 1;
      s.push_front( u );
      u.x += 1;
      s.push_front( u );
      u.x += 1;
      s.push_front( u );
    }

    void Dstar::getPredecessors( DStarState u, list<DStarState> &s )
    {
      s.clear();
      u.k.first = -1;
      u.k.second = -1;

      u.x += 1;
      if ( !occupied( u ) )
        s.push_front( u );
      u.y += 1;
      if ( !occupied( u ) )
        s.push_front( u );
      u.x -= 1;
      if ( !occupied( u ) )
        s.push_front( u );
      u.x -= 1;
      if ( !occupied( u ) )
        s.push_front( u );
      u.y -= 1;
      if ( !occupied( u ) )
        s.push_front( u );
      u.y -= 1;
      if ( !occupied( u ) )
        s.push_front( u );
      u.x += 1;
      if ( !occupied( u ) )
        s.push_front( u );
      u.x += 1;
      if ( !occupied( u ) )
        s.push_front( u );
    }

    void Dstar::updateStart( int x, int y )
    {
      s_start.x = x;
      s_start.y = y;

      k_m += heuristic( s_last, s_start );

      s_start = calculateKey( s_start );
      s_last = s_start;
    }

    void Dstar::updateGoal( int x, int y )
    {
      std::list< std::pair<MapPoint2, double> > toAdd;

      std::list< std::pair<MapPoint2, double> >::iterator kk;

      for (auto & i : cellHash)
      {
        if ( !math::equals( i.second.cost, C1 ) )
        {
          std::pair<MapPoint2, double> tp( i.first, i.second.cost );
          /*tp.first.x = i.first.x;
          tp.first.y = i.first.y;
          tp.second = i.second.cost;*/
          toAdd.push_back( tp );
        }
      }

      cellHash.clear();
      openHash.clear();

      while ( !openList.empty() )
        openList.pop();

      k_m = 0;

      s_goal.x = x;
      s_goal.y = y;

      DStarCell tmp;
      tmp.g = tmp.rhs = 0;
      tmp.cost = C1;

      cellHash[s_goal] = tmp;

      tmp.g = tmp.rhs = heuristic( s_start, s_goal );
      tmp.cost = C1;
      cellHash[s_start] = tmp;
      s_start = calculateKey( s_start );

      s_last = s_start;

      for ( kk = toAdd.begin(); kk != toAdd.end(); kk++ )
      {
        updateCell( kk->first.x, kk->first.y, kk->second );
      }
    }

    bool Dstar::replan()
    {
      path.clear();

      int res = computeShortestPath();
      if ( res < 0 )
        return false;

      list<DStarState> n;

      DStarState cur = s_start;

      if ( isinf( getG( s_start ) ) )
        return false;

      while ( cur != s_goal )
      {

        path.push_back( cur );
        getSuccessors( cur, n );

        if ( n.empty() )
          return false;

        double cmin = INFINITY;
        double tmin = INFINITY;
        DStarState smin;

        for (auto & i : n)
        {
          //if (occupied(*i)) continue;
          double val = cost( cur, i );
          double val2 = trueDist( i, s_goal ) + trueDist( s_start, i ); // (Euclidean) cost to goal + cost to pred
          val += getG( i );

          if ( math::equals( val, cmin ) )
          {
            if ( tmin > val2 )
            {
              tmin = val2;
              cmin = val;
              smin = i;
            }
          }
          else if ( val < cmin )
          {
            tmin = val2;
            cmin = val;
            smin = i;
          }
        }
        n.clear();
        cur = smin;
      }
      path.push_back( s_goal );
      return true;
    }

  }

}