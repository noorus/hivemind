#pragma once
#include "sc2_forward.h"
#include "hive_types.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"
#include "regiongraph.h"
#include "hive_rect2.h"
#include "map.h"

namespace hivemind {

  namespace pathfinding {

    struct ipoint2 {
      int x, y;
    };

    using MapPath = vector<MapPoint2>;

    struct GridGraphNode: public MapPoint2 {
    public:
      bool valid;
      bool closed;
    public:
      GridGraphNode(): MapPoint2( 0, 0 ), valid( false ), closed( false ) {}
      GridGraphNode( int x, int y ): MapPoint2( x, y ) {}
      inline void reset()
      {
        closed = false;
      }
      inline bool operator == ( const GridGraphNode& rhs ) const
      {
        return (x == rhs.x && y == rhs.y);
      }
      inline bool operator != ( const GridGraphNode& rhs ) const
      {
        return !(*this == rhs);
      }
      inline bool operator == ( const MapPoint2& rhs ) const
      {
        return (x == rhs.x && y == rhs.y);
      }
      inline bool operator != ( const MapPoint2& rhs ) const
      {
        return !(*this == rhs);
      }
    };

    class Dstar;

    class GridGraph {
    public:
      Array2<GridGraphNode> grid;
      Map & map_;
      int width_;
      int height_;
      bool useCreep_;
    public:
      GridGraph( Map& map );
      bool valid( const GridGraphNode& node ) const;
      void applyTo( Dstar* dstar );
      Real cost( const GridGraphNode& from, const GridGraphNode& to ) const;
      void reset();
    };

    MapPath pathAStarSearch( GridGraph& graph, const MapPoint2& start, const MapPoint2& end );

    // D* lite

    class state {
    public:
      int x;
      int y;
      std::pair<double, double> k;

      bool operator == ( const state &s2 ) const
      {
        return ((x == s2.x) && (y == s2.y));
      }

      bool operator != ( const state &s2 ) const
      {
        return ((x != s2.x) || (y != s2.y));
      }

      bool operator > ( const state &s2 ) const
      {
        if ( k.first - 0.00001 > s2.k.first ) return true;
        else if ( k.first < s2.k.first - 0.00001 ) return false;
        return k.second > s2.k.second;
      }

      bool operator <= ( const state &s2 ) const
      {
        if ( k.first < s2.k.first ) return true;
        else if ( k.first > s2.k.first ) return false;
        return k.second < s2.k.second + 0.00001;
      }


      bool operator < ( const state &s2 ) const
      {
        if ( k.first + 0.000001 < s2.k.first ) return true;
        else if ( k.first - 0.000001 > s2.k.first ) return false;
        return k.second < s2.k.second;
      }

    };

    struct cellInfo {

      double g;
      double rhs;
      double cost;

    };

    class state_hash {
    public:
      size_t operator()( const state &s ) const
      {
        return s.x + 34245 * s.y;
      }
    };


    typedef priority_queue<state, vector<state>, std::greater<state> > ds_pq;
    typedef unordered_map<state, cellInfo, state_hash, std::equal_to<> > ds_ch;
    typedef unordered_map<state, float, state_hash, std::equal_to<> > ds_oh;

    class Dstar {
    public:

      Dstar()
      {
        maxSteps = 80000;  // node expansions before we give up
        C1 = 1;      // cost of an unseen cell
      }
      void   init( int sX, int sY, int gX, int gY );
      void   updateCell( int x, int y, double val );
      void   updateStart( int x, int y );
      void   updateGoal( int x, int y );
      bool   replan();
      void   draw();
      void   drawCell( state s, float z );

      list<state> getPath()
      {
        return path;
      }

    private:

      list<state> path;

      double C1;
      double k_m;
      state s_start, s_goal, s_last;
      int maxSteps;

      ds_pq openList;
      ds_ch cellHash;
      ds_oh openHash;

      bool   close( double x, double y );
      void   makeNewCell( state u );
      double getG( state u );
      double getRHS( state u );
      void   setG( state u, double g );
      double setRHS( state u, double rhs );
      double eightCondist( state a, state b );
      int    computeShortestPath();
      void   updateVertex( state u );
      void   insert( state u );
      void   remove( state u );
      double trueDist( state a, state b );
      double heuristic( state a, state b );
      state  calculateKey( state u );
      void   getSucc( state u, list<state> &s );
      void   getPred( state u, list<state> &s );
      double cost( state a, state b );
      bool   occupied( state u );
      bool   isValid( state u );
      float  keyHashCode( state u )
      {
        return (float)(u.k.first + 1193 * u.k.second);
      }
    };

  }

}