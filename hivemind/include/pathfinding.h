#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"
#include "regiongraph.h"
#include "hive_rect2.h"
#include "map.h"

namespace hivemind {

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
      return ( x == rhs.x && y == rhs.y );
    }
    inline bool operator != ( const GridGraphNode& rhs ) const
    {
      return !( *this == rhs );
    }
    inline bool operator == ( const MapPoint2& rhs ) const
    {
      return ( x == rhs.x && y == rhs.y );
    }
    inline bool operator != ( const MapPoint2& rhs ) const
    {
      return !( *this == rhs );
    }
  };

  class GridGraph {
  public:
    Array2<GridGraphNode> grid;
    int width_;
    int height_;
    Map& map_;
  public:
    GridGraph( Map& map );
    bool valid( const GridGraphNode& node ) const;
    Real cost( const GridGraphNode& from, const GridGraphNode& to ) const;
    void reset();
  };

  MapPath pathAStarSearch( GridGraph& graph, const MapPoint2& start, const MapPoint2& end );

}