#pragma once
#include "sc2_forward.h"
#include "hive_types.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"
#include "regiongraph.h"
#include "hive_rect2.h"
#include "hive_minheap.h"
#include "map.h"

namespace hivemind {

  namespace pathfinding {

    using MapPath = vector<MapPoint2>;

    struct GridGraphNode: public MapPoint2 {
    public:
      bool valid;
      bool closed;
      // id
      // parents
      // children
      Real g;
      Real rhs;
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

    class GridGraph {
    public:
      Array2<GridGraphNode> grid;
      Map& map_;
      int width_;
      int height_;
      bool useCreep_;
      void process( Real initial_g = 0.0f, Real initial_rhs = 0.0f );
    public:
      GridGraph( Map& map );
      bool valid( const GridGraphNode& node ) const;
      Real cost( const GridGraphNode& from, const GridGraphNode& to ) const;
      inline GridGraphNode& node( int x, int y )
      {
        assert( x >= 0 && y >= 0 && x < width_ && y < height_ );
        return grid[x][y];
      }
      inline GridGraphNode& node( const MapPoint2& coord )
      {
        return node( coord.x, coord.y );
      }
      void reset();
    };

    const Real c_dstarEpsilon = 0.00001f;

    struct DStarLiteKey {
      Real first; // min(g(s), rhs(s)) + h(s_start, s) + k_m
      Real second; // min(g(s), rhs(s))
      DStarLiteKey(): first( 0.0f ), second( 0.0f )
      {
      }
      DStarLiteKey( Real first_, Real second_ ): first( first_ ), second( second_ )
      {
      }
      inline bool operator < ( const DStarLiteKey& rhs ) const
      {
        if ( first + c_dstarEpsilon < rhs.first )
          return true;
        else if ( first - c_dstarEpsilon > rhs.first )
          return false;
        return ( second < rhs.second );
      }
      inline bool operator > ( const DStarLiteKey& rhs ) const
      {
        if ( first - c_dstarEpsilon > rhs.first )
          return true;
        else if ( first + c_dstarEpsilon < rhs.first )
          return false;
        return ( second > rhs.second );
      }
    };

    class DStarLite {
    private:
      GridGraph graph_;
      uint64_t start_;
      uint64_t goal_;
      Heap<uint64_t, DStarLiteKey> U;
      Real k_m;
      vector<uint64_t> predecessors( GridGraphNode& s );
      vector<uint64_t> successors( GridGraphNode& s );
    public:
      void initialize( const MapPoint2& from, const MapPoint2& to );
      DStarLiteKey calculateKey( uint64_t s, Real km );
      void updateVertex( uint64_t u );
      void computeShortestPath();
    };

    MapPath pathAStarSearch( GridGraph& graph, const MapPoint2& start, const MapPoint2& end );

  }

}