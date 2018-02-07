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

    class DStarState {
    public:
      int x;
      int y;
      std::pair<double, double> k;
      bool operator == ( const DStarState &s2 ) const
      {
        return ((x == s2.x) && (y == s2.y));
      }
      bool operator != ( const DStarState &s2 ) const
      {
        return ((x != s2.x) || (y != s2.y));
      }
      bool operator > ( const DStarState &s2 ) const
      {
        if ( k.first - 0.00001 > s2.k.first ) return true;
        else if ( k.first < s2.k.first - 0.00001 ) return false;
        return k.second > s2.k.second;
      }
      bool operator <= ( const DStarState &s2 ) const
      {
        if ( k.first < s2.k.first ) return true;
        else if ( k.first > s2.k.first ) return false;
        return k.second < s2.k.second + 0.00001;
      }
      bool operator < ( const DStarState &s2 ) const
      {
        if ( k.first + 0.000001 < s2.k.first ) return true;
        else if ( k.first - 0.000001 > s2.k.first ) return false;
        return k.second < s2.k.second;
      }
    };

    struct DStarCell
    {
      double g;
      double rhs;
      double cost;
    };

    class DStarHash {
    public:
      size_t operator()( const DStarState &s ) const
      {
        return s.x + 34245 * s.y;
      }
    };

    using DStarPath = list<DStarState>;

    typedef priority_queue<DStarState, vector<DStarState>, std::greater<DStarState> > ds_pq;
    using DStarCellHashMap = unordered_map<DStarState, DStarCell, DStarHash, std::equal_to<> >;
    using DStarOpenHashMap = unordered_map<DStarState, float, DStarHash, std::equal_to<> >;

    class Dstar {
    private:
      DStarPath path;
      double C1;
      double k_m;
      DStarState s_start, s_goal, s_last;
      int maxSteps;
      ds_pq openList;
      DStarCellHashMap cellHash;
      DStarOpenHashMap openHash;
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
      inline const DStarPath& getPath() { return path; }
    private:
      bool   close( double x, double y );
      void   makeNewCell( DStarState u );
      double getG( DStarState u );
      double getRHS( DStarState u );
      void   setG( DStarState u, double g );
      double setRHS( DStarState u, double rhs );
      double eightCondist( DStarState a, DStarState b );
      int    computeShortestPath();
      void   updateVertex( DStarState u );
      void   insert( DStarState u );
      void   remove( DStarState u );
      double trueDist( DStarState a, DStarState b );
      double heuristic( DStarState a, DStarState b );
      DStarState  calculateKey( DStarState u );
      void   getSucc( DStarState u, list<DStarState> &s );
      void   getPred( DStarState u, list<DStarState> &s );
      double cost( DStarState a, DStarState b );
      bool   occupied( DStarState u );
      bool   isValid( DStarState u );
      float  keyHashCode( DStarState u )
      {
        return (float)(u.k.first + 1193 * u.k.second);
      }
    };

  }

}