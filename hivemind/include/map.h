#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "distancemap.h"
#include "regiongraph.h"
#include "baselocation.h"

namespace hivemind {

  class Bot;
  struct BaseLocation;

  enum MapFlag {
    MapFlag_Walkable = 1,
    MapFlag_Buildable = 2,
    MapFlag_ResourceBox = 4, //!< Is this tile part of a base location's harvesting area
    MapFlag_InnerWalkable = 8, //!< A walkable tile at least 2 tiles away from any non-walkable tile
    MapFlag_Ramp = 16, //!< A tile that is part of a ramp
    MapFlag_NearRamp = 32 //!< A tile that is at most 2 tiles away from a ramp tile
  };

  struct MapPoint2 {
    int x;
    int y;
    MapPoint2( int x_, int y_ ): x( x_ ), y( y_ )
    {
    }
    inline MapPoint2& operator = ( const Vector2& rhs )
    {
      x = math::floor( rhs.x );
      y = math::floor( rhs.y );
      return *this;
    }
    inline MapPoint2& operator = ( const sc2::Point2D& rhs )
    {
      x = math::floor( rhs.x );
      y = math::floor( rhs.y );
      return *this;
    }
    inline MapPoint2& operator = ( const sc2::Point3D& rhs )
    {
      x = math::floor( rhs.x );
      y = math::floor( rhs.y );
      return *this;
    }
    inline operator sc2::Point2DI() const
    {
      sc2::Point2DI ret;
      ret.x = x;
      ret.y = y;
      return ret;
    }
    inline operator sc2::Point2D() const
    {
      sc2::Point2D ret;
      ret.x = ( (Real)x + 0.5f );
      ret.y = ( (Real)y + 0.5f );
      return ret;
    }
    inline operator Vector2() const
    {
      return Vector2(
        ( (Real)x + 0.5f ),
        ( (Real)y + 0.5f )
      );
    }
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

  struct Creep {
    int label;
    Contour contour;
    vector<vector<MapPoint2>> fronts;
  };

  using CreepVector = vector<Creep>;

  enum CreepTile: uint8_t {
    CreepTile_No = 0,
    CreepTile_Walkable,
    CreepTile_Buildable
  };

  struct BuildingReservation {
    Point2DI position_;
    UnitTypeID type_;
    BuildingReservation( const Point2DI& pos = Point2DI(), UnitTypeID t = 0 ): position_( pos ), type_( t ) {}
  };

  using BuildingReservationMap = std::map<uint64_t, BuildingReservation>; // using encodePoint()

  class Map {
  public:
    Bot* bot_;
    size_t width_; //!< Map width
    size_t height_; //!< Map height
    Array2<uint64_t> flagsMap_; //!< Static walkable & buildable flags
    Array2<Real> heightMap_; //!< Map heights
    Array2<int> labelsMap_; //!< Map component labels
    Array2<bool> creepMap_; //!< Current creep spread visible to us
    Array2<CreepTile> zergBuildable_; //!< Space that is currently buildable to us
    Array2<int> labeledCreeps_; //!< Contour-traced buildable creeps by label (index)
    Array2<bool> reservedMap_; //!< Spots that are used up by building footprints
    BuildingReservationMap buildingReservations_; //!< Our own to-build structure footprints
    vector<MapPoint2> creepTumors_;
    CreepVector creeps_;
    uint8_t* contourTraceImageBuffer_;
    ComponentVector components_; //!< Map components
    PolygonComponentVector polygons_;
    Real maxZ_; //!< Highest terrain Z coordinate in the map
    mutable std::map<std::pair<size_t, size_t>, DistanceMap> distanceMapCache_;
    vector<UnitVector> resourceClusters_;
    Analysis::RegionGraph graph_;
    Analysis::RegionGraph graphSimplified_;
    PolygonComponentVector obstacles_;
    std::map<Analysis::RegionNodeID, Analysis::Chokepoint> chokepointSides_;
    BaseLocationVector baseLocations_;
  private:
    bool rampHasCreepTumor( int x, int y );
    void updateReservedMap();
    void splitCreepFronts();
    void labelBuildableCreeps();
    void markResourceBoxes(); //!< Set MapFlag_ResourceBox values based on base locations
  public:
    Map( Bot* bot );
    ~Map();
    void rebuild();
    void draw();
    bool updateCreep();
    bool hasCreep( size_t x, size_t y ) { return ( labeledCreeps_[x][y] > 0 ); }
    bool hasCreep( const Vector2& position ) { return hasCreep( (size_t)position.x, (size_t)position.y ); }
    Creep* creep( size_t x, size_t y );
    Creep* creep( const Vector2& position ) { return creep( (size_t)position.x, (size_t)position.y ); }
    bool updateZergBuildable(); // Make sure that creep is up to date first
    bool canZergBuild( UnitTypeID structure, size_t x, size_t y, int padding = 0 );
    inline bool canZergBuild( UnitTypeID structure, const Vector2& pos, int padding = 0 ) { return canZergBuild( structure, (size_t)pos.x, (size_t)pos.y, padding ); }
    const size_t width() const { return width_; }
    const size_t height() const { return height_; }
    BaseLocation* closestLocation( const Vector2& position );
    Vector2 closestByGround( const Vector2& from, const list<Vector2>& to );
    Path shortestScoutPath( const Vector2& start, vector<Vector2>& locations );
    void reserveFootprint( const Point2DI& position, UnitTypeID type );
    void clearFootprint( const Point2DI& position );
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