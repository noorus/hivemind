#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "distancemap.h"
#include "regiongraph.h"
#include "baselocation.h"
#include "debug.h"

namespace hivemind {

  class Bot;
  struct BaseLocation;

  enum MapFlag {
    MapFlag_Walkable = 1,
    MapFlag_Buildable = 2,
    MapFlag_ResourceBox = 4, //!< Is this tile part of a base location's harvesting area
    MapFlag_InnerWalkable = 8, //!< A walkable tile at least 2 tiles away from any non-walkable tile
    MapFlag_Ramp = 16, //!< A tile that is part of a ramp
    MapFlag_NearRamp = 32, //!< A tile that is at most 2 tiles away from a ramp tile
    MapFlag_StartLocation = 64, //!< A tile that is part of a start location's footprint
    MapFlag_NearStartLocation = 128, //!< A tile that is at most 2 tiles away from a base location footprint tile
    MapFlag_VespeneGeyser = 256 //!< A tile that is part of a vespene geyser
  };

  struct MapComponent {
    int label; //!< Component number
    Contour contour;
    ContourVector holes;
  };

  using ComponentVector = vector<MapComponent>;

  struct PolygonComponent {
    int label;
    Polygon contour;
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

  class Region;

  using RegionSet = set<Region*>;
  using RegionVector = vector<Region*>;

  class Region {
  public:
    int label_;
    Polygon polygon_;
    Analysis::ChokeSidesSet chokepoints_;
    RegionSet reachableRegions_;
    Real opennessDistance_;
    Vector2 opennessPoint_;
  };

  class Map {
  public:
    enum ReservedTile: uint8_t {
      Reserved_None = 0,
      Reserved_NearResource,
      Reserved_Reserved
    };
    Bot* bot_;
    size_t width_; //!< Map width
    size_t height_; //!< Map height
    Array2<uint64_t> flagsMap_; //!< Static walkable & buildable flags
    Array2<Real> heightMap_; //!< Map heights
    Array2<int> labelsMap_; //!< Map component labels
    Array2<bool> creepMap_; //!< Current creep spread visible to us
    Array2<CreepTile> zergBuildable_; //!< Space that is currently buildable to us
    Array2<int> labeledCreeps_; //!< Contour-traced buildable creeps by label (index)
    Array2<ReservedTile> reservedMap_; //!< Spots that are used up by building footprints
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
    std::map<Analysis::RegionNodeID, Analysis::ChokeSides> chokepointSides_;
    BaseLocationVector baseLocations_;
    RegionVector regions_;
    Array2<int> regionMap_;
  private:
    bool rampHasCreepTumor( int x, int y );
    void updateReservedMap();
    void splitCreepFronts();
    void labelBuildableCreeps();
    void drawPoly( DebugExtended& debug, Polygon& poly, sc2::Color color );
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
    bool canZergBuild( UnitTypeID structure, size_t x, size_t y, int padding = 0, bool nearResources = true, bool noMainOverlap = true, bool notNearMain = true, bool notNearRamp = true );
    bool isBuildable( const MapPoint2& position, UnitTypeID structure, bool nearResources = true );
    bool findClosestBuildablePosition( MapPoint2& position, UnitTypeID structure, bool nearResources = true );
    inline bool canZergBuild( UnitTypeID structure, const Vector2& pos, int padding = 0, bool nearResources = true, bool noMainOverlap = true, bool notNearMain = true, bool notNearRamp = true ) { return canZergBuild( structure, (size_t)pos.x, (size_t)pos.y, padding, nearResources, noMainOverlap, notNearMain, notNearRamp ); }
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