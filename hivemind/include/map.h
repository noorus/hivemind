#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "distancemap.h"
#include "regiongraph.h"

namespace hivemind {

  class Bot;
  struct BaseLocation;

  enum MapFlag {
    MapFlag_Walkable = 1,
    MapFlag_Buildable = 2
  };

  struct MapPoint2 {
    int x;
    int y;
    MapPoint2( int x_, int y_ ): x( x_ ), y( y_ ) {}
  };

  using Contour = vector<MapPoint2>;
  using ContourVector = vector<Contour>;

  struct MapComponent {
    int label; //!< Component number
    Contour contour;
    ContourVector holes;
  };

  using ComponentVector = vector<MapComponent>;

  struct PolygonComponent {
    int label;
    Polygon contour;
    PolygonVector holes;
  };

  using PolygonComponentVector = vector<PolygonComponent>;

  class Map {
  public:
    Bot* bot_;
    size_t width_; //!< Map width
    size_t height_; //!< Map height
    Array2<uint64_t> flagsMap_; //!< Map walkable & buildable flags
    Array2<Real> heightMap_; //!< Map heights
    Array2<int> labelsMap_;
    ComponentVector components_;
    PolygonComponentVector polygons_;
    Real maxZ_;
    mutable std::map<std::pair<size_t, size_t>, DistanceMap> distanceMapCache_;
    vector<UnitVector> resourceClusters_;
    Analysis::RegionGraph graph_;
    Analysis::RegionGraph graphSimplified_;
    PolygonComponentVector obstacles_;
    std::map<Analysis::nodeID, Analysis::chokeSides_t> chokepointSides_;
  public:
    Map( Bot* bot );
    void rebuild();
    void draw();
    const size_t width() const { return width_; }
    const size_t height() const { return height_; }
    bool isValid( size_t x, size_t y ) const;
    bool isValid( const Vector2& position ) const { return isValid( (size_t)position.x, (size_t)position.y ); }
    bool isPowered( const Vector2& position ) const;
    bool isExplored( const Vector2& position ) const;
    bool isVisible( const Vector2& position ) const;
    bool isWalkable( size_t x, size_t y );
    bool isWalkable( const Vector2& position ) { return isWalkable( (size_t)position.x, (size_t)position.y ); }
    bool isBuildable( size_t x, size_t y );
    bool isBuildable( const Vector2& position ) { return isBuildable( (size_t)position.x, (size_t)position.y ); }
    const DistanceMap& getDistanceMap( const Point2D& tile ) const;
  };

}