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

#define HIVE_PATHING_USE_FIXED_POINT 1

#if HIVE_PATHING_USE_FIXED_POINT
    typedef int PathCost;
#else
    typedef Real PathCost;
#endif

    struct GridGraphNode {
    public:
      MapPoint2 location;

      bool valid;
      PathCost g;
      PathCost rhs;
      bool hasObstacle;

    public:
      GridGraphNode():
        location( 0, 0 ),
        valid( false ),
        hasObstacle( false )
      {
      }
      GridGraphNode( int x, int y ):
        location( x, y ),
        valid( false ),
        hasObstacle( false )
      {
      }
      inline bool operator == ( const GridGraphNode& rhs ) const
      {
        return location == rhs.location;
      }
      inline bool operator != ( const GridGraphNode& rhs ) const
      {
        return !(*this == rhs);
      }
      inline bool operator == ( const MapPoint2& rhs ) const
      {
        return location == rhs;
      }
      inline bool operator != ( const MapPoint2& rhs ) const
      {
        return !(*this == rhs);
      }
    };

    class GridMap
    {
    public:
      virtual ~GridMap() = default;
      virtual int width() const = 0;
      virtual int height() const = 0;
      virtual bool isWalkable(int x, int y) const = 0;
    };

    class GridMapAdaptor: public pathfinding::GridMap
    {
    public:
      Map* map_;

      explicit GridMapAdaptor(Map* map):
        map_(map)
      {
      }

      virtual int width() const override
      {
        return (int)map_->width();
      }
      virtual int height() const override
      {
        return (int)map_->height();
      }
      virtual bool isWalkable(int x, int y) const
      {
         return map_->flagsMap_[x][y] & MapFlag_Walkable;
      }
    };

    class GridGraph {
    public:
      Array2<GridGraphNode> grid;
      std::unique_ptr<GridMap> map_;
      Console* console_;
      int width_;
      int height_;

      void initialize();

    public:

      explicit GridGraph( Console* console, std::unique_ptr<GridMap> map );

      PathCost cost( const GridGraphNode& from, const GridGraphNode& to ) const;

      inline GridGraphNode& node( int x, int y )
      {
        assert( x >= 0 && y >= 0 && x < width_ && y < height_ );
        return grid[x][y];
      }
      inline const GridGraphNode& node( int x, int y ) const
      {
        assert( x >= 0 && y >= 0 && x < width_ && y < height_ );
        return grid[x][y];
      }

      inline GridGraphNode& node( const MapPoint2& coord )
      {
        return node( coord.x, coord.y );
      }
      inline const GridGraphNode& node( const MapPoint2& coord ) const
      {
        return node( coord.x, coord.y );
      }
    };

    struct DStarLiteKey {
      PathCost first; // min(g(s), rhs(s)) + h(s_start, s) + k_m
      PathCost second; // min(g(s), rhs(s))

      DStarLiteKey():
        first( 0 ),
        second( 0 )
      {
      }
      DStarLiteKey( PathCost first_, PathCost second_ ):
        first( first_ ),
        second( second_ )
      {
      }
      bool operator < ( const DStarLiteKey& rhs ) const
      {
        return std::tie(first, second) < std::tie(rhs.first, rhs.second);
      }
      bool operator > ( const DStarLiteKey& rhs ) const
      {
        return rhs < *this;
      }
    };

    typedef MapPoint2 NodeIndex;


    class DStarLite;
    typedef std::unique_ptr<DStarLite> DStarLitePtr;

    class DStarLite {
    public:
      std::unique_ptr<pathfinding::GridGraph> graph_;
    private:
      NodeIndex start_;
      NodeIndex goal_;
      Heap<NodeIndex, DStarLiteKey> U;
      PathCost k_m;

      DStarLiteKey calculateKey( NodeIndex s, PathCost km );
      void updateVertex( NodeIndex u );

      void initialize( const MapPoint2& start, const MapPoint2& goal );

      explicit DStarLite(Console* console, std::unique_ptr<GridMap> map, const MapPoint2& start, const MapPoint2& goal);

    public:

      static DStarLitePtr search(Console* console, std::unique_ptr<GridMap> map, const MapPoint2& start, const MapPoint2& goal);

      void updateWalkability(MapPoint2 changedNode, bool hasObstacle);
      void computeShortestPath();

      pair<PathCost, NodeIndex> getNext(NodeIndex current) const;
      PathCost getNextValue(NodeIndex current) const;
      NodeIndex getNextNode(NodeIndex current) const;

      MapPath getMapPath() const;
    };
  }

}
