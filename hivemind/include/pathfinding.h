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
    Real weight;
    bool valid;
    bool closed;
  public:
    GridGraphNode(): MapPoint2( 0, 0 ), weight( 9999.0f ), valid( false ), closed( false ) {}
    GridGraphNode( int x, int y ): MapPoint2( x, y ) {}
    inline Real cost() const
    {
      return weight;
    }
    inline Real cost( const GridGraphNode& neighbor ) const
    {
      if ( neighbor.x != x && neighbor.y != y )
        return ( weight * 1.41421f );
      else
        return weight;
    }
    inline const bool wall() const { return !valid; }
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
    int width;
    int height;
  public:
    GridGraph( Map& map ): width( (int)map.width() ), height( (int)map.height() )
    {
      grid.resize( width, height );
      for ( int x = 0; x < width; ++x )
        for ( int y = 0; y < height; ++y )
        {
          GridGraphNode node( x, y );
          node.valid = ( map.flagsMap_[x][y] & MapFlag_Walkable );
          node.closed = false;
          node.weight = 1.0f;
          grid[x][y] = node;
        }
    }
  };

  MapPath pathAStarSearch( GridGraph& graph, const MapPoint2& start, const MapPoint2& end );

}