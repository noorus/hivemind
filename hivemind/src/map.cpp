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

HIVE_DECLARE_CONVAR( creep_updateinterval, "Delay between map creep spread/buildability updates.", 40 );
HIVE_DECLARE_CONVAR( analysis_invertwalkable, "Whether to invert the detected walkable area polygons. Try this if terrain analysis fails for a map.", false );
HIVE_DECLARE_CONVAR( analysis_verbose, "Print verbose status messages on map analysis.", true );
HIVE_DECLARE_CONVAR( analysis_use_cache, "Whether to cache expensive map analysis data.", true );

HIVE_DECLARE_CONVAR( draw_map, "Whether to draw map analysis debug features.", true );
HIVE_DECLARE_CONVAR( draw_baselocations, "Whether to show base location debug information.", true );
HIVE_DECLARE_CONVAR( draw_chokepoints, "Whether to draw chokepoint debug features.", true );

HIVE_DECLARE_CONVAR( creep_debug, "Whether to draw creep debug features.", false );

#ifdef HIVE_SUPPORT_MAP_DUMPS
HIVE_DECLARE_CONVAR( analysis_dump_maps, "Dump out images of generated map layers upon analysis.", false );
#endif

namespace hivemind {

  const size_t c_legalActions = 4;
  const int c_actionX[c_legalActions] = { 1, -1, 0, 0 };
  const int c_actionY[c_legalActions] = { 0, 0, 1, -1 };

  const char cClosestRegionTilesCacheName[] = "regionclosetiles";
  const char cRegionLabelMapCacheName[] = "regionlabels";
  const char cRegionVectorCacheName[] = "regions";
  const char cFlagsMapCacheName[] = "flagtiles";
  const char cChokepointsCacheName[] = "chokes";

  Map::Map( Bot* bot ): bot_( bot ), width_( 0 ), height_( 0 ), contourTraceImageBuffer_( nullptr )
  {
  }

  Map::~Map()
  {
    for ( auto regptr : regions_ )
      delete regptr;
    regions_.clear();
    if ( contourTraceImageBuffer_ )
      ::free( contourTraceImageBuffer_ );
  }

  template <typename Call>
  bool util_verbosePerfSection( Bot* bot, const string& description, Call c )
  {
    if ( !g_CVar_analysis_verbose.as_b() )
    {
      return c();
    }
    else
    {
      platform::PerformanceTimer timer;
      bot->console().printf( "%s...", description.c_str() );
      timer.start();
      bool ret = c();
      if ( ret )
        bot->console().printf( "%s... took %.5f ms", description.c_str(), timer.stop() );
      else
        bot->console().printf( "%s... failed", description.c_str() );
      return ret;
    }
  }

  void Map::rebuild( const MapData& data )
  {
    bot_->console().printf( "Map: Rebuilding..." );

    info_ = data;

    // These could be different in the future
    bool readCache = g_CVar_analysis_use_cache.as_b();
    bool writeCache = g_CVar_analysis_use_cache.as_b();

    bool dumpImages = false;
  #ifdef HIVE_SUPPORT_MAP_DUMPS
    dumpImages = g_CVar_analysis_dump_maps.as_b();
  #endif

    distanceMapCache_.clear();

    const GameInfo& info = bot_->observation().GetGameInfo();

    util_verbosePerfSection( bot_, "Map: Extracting walkability- and heightmaps", [&]
    {
      Analysis::Map_BuildBasics( bot_->observation(), width_, height_, flagsMap_, heightMap_ );

      for ( auto unit : bot_->observation().GetUnits( Unit::Alliance::Neutral ) )
        maxZ_ = std::max( unit->pos.z, maxZ_ );

      return true;
    } );

    if ( dumpImages )
      bot_->debug().mapDumpBasicMaps( flagsMap_, heightMap_, info );

    creepMap_.resize( width_, height_ );
    creepMap_.reset( false );
    zergBuildable_.resize( width_, height_ );
    zergBuildable_.reset( CreepTile_No );
    labeledCreeps_.resize( width_, height_ );
    labeledCreeps_.reset( 0 );
    reservedMap_.resize( width_, height_ );
    reservedMap_.reset( Reserved_None );
    regionMap_.resize( width_, height_ );
    regionMap_.reset( -1 );

    if ( contourTraceImageBuffer_ )
      ::free( contourTraceImageBuffer_ );

    contourTraceImageBuffer_ = ( uint8_t* )::malloc( width_ * height_ );

    ComponentVector components;
    PolygonComponentVector polygons;

    obstacles_.clear();

    for ( auto regptr : regions_ )
      delete regptr;
    regions_.clear();

    Array2<int> componentLabels;

    bool gotRegions = false;
    bool gotClosestRegionTiles = false;
    bool gotFlagsMap = false;

    util_verbosePerfSection( bot_, "Map: Processing contours", [&]
    {
      Analysis::Map_ProcessContours( flagsMap_, componentLabels, components );

      if ( dumpImages )
        bot_->debug().mapDumpLabelMap( componentLabels, true, "components" );

      return true;
    } );

    // Polygon generation from traced contours ---

    util_verbosePerfSection( bot_, "Map: Generating polygons", [&]
    {
      Analysis::Map_ComponentPolygons( components, polygons );

      if ( g_CVar_analysis_invertwalkable.as_b() )
      {
        bot_->console().printf( "Map: Inverting walkable polygons to obstacles..." );
        Analysis::Map_InvertPolygons( polygons, obstacles_, Rect2( info.playable_min, info.playable_max ), Vector2( (Real)info.width, (Real)info.height ) );
      } else
        obstacles_ = polygons;

      return true;
    } );

    // Try to load flagsmap

    if ( readCache && Cache::hasMapCache( data, cFlagsMapCacheName ) )
    {
      gotFlagsMap = util_verbosePerfSection( bot_, "Map: Loading ground flags tilemap (cached)", [&]
      {
        return ( Cache::mapReadUint64Array2( data, flagsMap_, cFlagsMapCacheName ) );
      } );
    }

    // Try to load regions + regionlabelmaps

    if ( readCache && Cache::hasMapCache( data, cRegionVectorCacheName ) && Cache::hasMapCache( data, cRegionLabelMapCacheName ) )
    {
      assert( regions_.empty() );
      gotRegions = util_verbosePerfSection( bot_, "Map: Loading regions (cached)", [&]
      {
        return ( Cache::mapReadRegionVector( data, regions_, cRegionVectorCacheName )
          && Cache::mapReadIntArray2( data, regionMap_, cRegionLabelMapCacheName )
          && Cache::mapReadChokeVector( data, chokepoints_, regions_, cChokepointsCacheName ) );
      } );
    }

    Analysis::RegionChokesMap tempChokeSides;
    Analysis::RegionGraph simplifiedGraph;

    // All of the stuff inside here is only needed if we couldn't load complete regions from the cache.
    if ( !gotRegions )
    {
      // Voronoi graph generation & pruning ---

      Analysis::RegionGraph graph;
      bgi::rtree<BoostSegmentI, bgi::quadratic<16>> rtree;

      util_verbosePerfSection( bot_, "Map: Generating Voronoi diagram", [&]
      {
        Analysis::Map_MakeVoronoi( info, obstacles_, componentLabels, graph, rtree );
        Analysis::Map_PruneVoronoi( graph );
        return true;
      } );

      // Graph processing ---

      util_verbosePerfSection( bot_, "Map: Processing graph", [&]
      {
        Analysis::Map_DetectNodes( graph );
        Analysis::Map_SimplifyGraph( graph, simplifiedGraph );
        Analysis::Map_MergeRegionNodes( simplifiedGraph );
        return true;
      } );

      // Chokepoints ---

      util_verbosePerfSection( bot_, "Map: Figuring out chokepoints", [&]
      {
        Analysis::Map_GetChokepointSides( simplifiedGraph, rtree, tempChokeSides );
        return true;
      } );

      if ( dumpImages )
        bot_->debug().mapDumpPolygons( width_, height_, obstacles_, tempChokeSides );

      // Region splitting ---

      util_verbosePerfSection( bot_, "Map: Splitting region polygons", [&]
      {
        Analysis::Map_MakeRegions( polygons, tempChokeSides, flagsMap_, width_, height_, regions_, regionMap_, simplifiedGraph );
        return true;
      } );
    }

    // Closest-region tile cache ---

    closestRegionMap_.resize( width_, height_ );
    closestRegionMap_.reset( -1 );

    if ( readCache && Cache::hasMapCache( data, cClosestRegionTilesCacheName ) )
    {
      gotClosestRegionTiles = util_verbosePerfSection( bot_, "Map: Loading closest-region tilemap (cached)", [&]
      {
        return ( Cache::mapReadIntArray2( data, closestRegionMap_, cClosestRegionTilesCacheName ) );
      } );
    }

    if ( !gotClosestRegionTiles )
    {
      util_verbosePerfSection( bot_, "Map: Precalculating closest-region tilemap", [&]
      {
        Analysis::Map_CacheClosestRegions( regions_, regionMap_, closestRegionMap_ );
        if ( writeCache )
          Cache::mapWriteIntArray2( data, closestRegionMap_, cClosestRegionTilesCacheName );

        return true;
      } );
    }

    if ( dumpImages )
      bot_->debug().mapDumpLabelMap( closestRegionMap_, false, "closest_tiles" );

    // Resource clusters ---

    vector<UnitVector> resourceClusters;

    util_verbosePerfSection( bot_, "Map: Finding resource clusters", [&]
    {
      Analysis::Map_FindResourceClusters( bot_->observation(), resourceClusters, 4, 16.0f );

      updateReservedMap();

      return true;
    } );

    // Base locations ---

    util_verbosePerfSection( bot_, "Map: Creating base locations", [&]
    {
      baseLocations_.clear();

      size_t index = 0;
      for ( auto& cluster : resourceClusters )
        baseLocations_.emplace_back( bot_, index++, cluster );

      if ( !gotFlagsMap )
      {
        Analysis::Map_MarkBaseTiles( flagsMap_, baseLocations_ );
        if ( writeCache )
          Cache::mapWriteUint64Array2( data, flagsMap_, cFlagsMapCacheName );
      }

      return true;
    } );

    if ( dumpImages )
      bot_->debug().mapDumpBaseLocations( flagsMap_, resourceClusters, info, baseLocations_ );

    if ( !gotRegions )
    {
      util_verbosePerfSection( bot_, "Map: Calculating region heights", [&]
      {
        Analysis::Map_CalculateRegionHeights( flagsMap_, regions_, regionMap_, heightMap_ );

        chokepoints_.clear();
        Analysis::Map_ConnectChokepoints( tempChokeSides, regions_, closestRegionMap_, simplifiedGraph, chokepoints_ );

        if ( writeCache )
        {
          Cache::mapWriteRegionVector( data, regions_, cRegionVectorCacheName );
          Cache::mapWriteIntArray2( data, regionMap_, cRegionLabelMapCacheName );
          Cache::mapWriteChokeVector( data, chokepoints_, cChokepointsCacheName );
        }

        return true;
      } );
    }

    bot_->console().printf( "Map: Rebuild done" );
  }

  void Map::draw()
  {
    // draw map tile flags and region polygons if draw_map is enabled
    if ( g_CVar_draw_map.as_b() )
    {
      Point2D camera = bot_->observation().GetCameraPos();
      for ( float x = camera.x - 16.0f; x < camera.x + 16.0f; ++x )
      {
        for ( float y = camera.y - 16.0f; y < camera.y + 16.0f; ++y )
        {
          auto ix = math::floor( x );
          auto iy = math::floor( y );

          if ( ix < 0 || iy < 0 || ix >= width_ || iy >= height_ )
            continue;

          sc2::Point3D pos( (Real)ix + 0.5f, (Real)iy + 0.5f, heightMap_[ix][iy] + 0.25f );
          auto tile = flagsMap_[ix][iy];
          if ( tile & MapFlag_StartLocation )
            bot_->debug().drawSphere( pos, 0.1f, sc2::Colors::Green );
          else if ( tile & MapFlag_NearStartLocation )
            bot_->debug().drawSphere( pos, 0.1f, sc2::Colors::Teal );
          else if ( tile & MapFlag_Ramp )
            bot_->debug().drawSphere( pos, 0.1f, sc2::Colors::Purple );
          else if ( tile & MapFlag_NearRamp )
            bot_->debug().drawSphere( pos, 0.1f, sc2::Colors::Yellow );
          else if ( tile & MapFlag_VespeneGeyser )
            bot_->debug().drawSphere( pos, 0.1f, sc2::Colors::Green );
          else if ( tile & MapFlag_VisionBlocker )
            bot_->debug().drawSphere( pos, 0.1f, sc2::Colors::Blue );
          else if ( tile & MapFlag_NearVisionBlocker )
            bot_->debug().drawSphere( pos, 0.1f, sc2::Colors::Gray );
        }
      }

      size_t i = 0;
      for ( auto& regptr : regions_ )
      {
        auto color = utils::prettyColor( i );
        bot_->debug().drawMapPolygon( *this, regptr->polygon_, color );
        char asd[64];
        sprintf_s( asd, 64, "region %d\r\nheight %.3f\r\nlevel %d", regptr->label_, regptr->height_, regptr->heightLevel_ );
        auto mid = regptr->polygon_.centroid();
        string str = asd;
        for ( auto choke : regptr->chokepoints_ )
          str.append( "\r\nchoke: " + std::to_string( choke ) );
        bot_->debug().drawText( str, Vector3( mid.x, mid.y, regptr->height_ + 0.5f ), Colors::White, 12 );
        i++;
      }
    }

    if ( g_CVar_draw_chokepoints.as_b() )
    {
      for ( auto& choke : chokepoints_ )
      {
        bot_->debug().drawMapLine( *this, choke.side1, choke.side2, Colors::Green );
        char str[64];
        sprintf_s( str, 64, "chokepoint %d\r\nregions %d & %d", choke.id_, choke.region1->label_, choke.region2->label_ );
        auto mid = choke.middle().to3( maxZ_ );
        bot_->debug().drawText( str, mid, sc2::Colors::Green, 12 );
      }
    }
    /*for ( auto& node : graphSimplified_.nodes )
    {
      auto pt = util_tileToMarker( node, heightMap_, 1.0f, maxZ_ );
      bot_->debug().drawSphere( pt, 0.25f, sc2::Colors::Teal );
    }

    for ( size_t id = 0; id < simplifiedGraph.adjacencyList.size(); id++ )
    {
      for ( auto adj : simplifiedGraph.adjacencyList[id] )
      {
        auto v0 = simplifiedGraph.nodes[id].to3( maxZ_ );
        auto v1 = simplifiedGraph.nodes[adj].to3( maxZ_ );
        bot_->debug().drawLine( v0, v1, sc2::Colors::Teal );
      }
    }*/
    // draw baselocation info if draw_baselocations is enabled
    if ( g_CVar_draw_baselocations.as_b() )
    {
      for ( auto& location : baseLocations_ )
      {
        char msg[64];
        sprintf_s( msg, 64, "Base location %zd (region %d)", location.baseID_, location.region_ );
        bot_->debug().drawText( msg, Vector3( location.position_.x, location.position_.y, maxZ_ ), sc2::Colors::White );
        Real height = heightMap_[math::floor( location.position_.x )][math::floor( location.position_.y )];
        bot_->debug().drawBox(
          Vector3( location.left_, location.top_, height ),
          Vector3( location.right_, location.bottom_, height + 1.0f ), sc2::Colors::White );
        auto pos = Vector3( location.position_.x, location.position_.y, height );
        bot_->debug().drawSphere( pos, 1.0f, sc2::Colors::White );
        bot_->debug().drawSphere( pos, 2.0f, sc2::Colors::White );
        bot_->debug().drawSphere( pos, 3.0f, sc2::Colors::White );
      }
    }

    // draw creep fronts if creep_debug is enabled
    if ( g_CVar_creep_debug.as_b() )
    {
      for ( auto& creep : creeps_ )
        for ( size_t i = 0; i < creep.fronts.size(); i++ )
        {
          auto color = utils::prettyColor( i, 40 );
          for ( auto& pt : creep.fronts[i] )
            bot_->debug().drawSphere( Vector3( (Real)pt.x + 0.5f, (Real)pt.y + 0.5f, heightMap_[pt.x][pt.y] + 0.5f ), 0.1f, color );
        }
    }
  }

  bool Map::updateCreep()
  {
    auto& rawData = bot_->observation().GetRawObservation()->raw_data();
    if ( !rawData.has_map_state() || !rawData.map_state().has_creep() )
      return false;

    creepMap_.reset( false );

    auto& creep = rawData.map_state().creep().data();
    for ( size_t x = 0; x < width_; x++ )
      for ( size_t y = 0; y < height_; y++ )
        creepMap_[x][y] = ( creep[x + ( height_ - 1 - y ) * width_] ? true : false );

    return true;
  }

  void Map::gameBegin()
  {
    nextCreepUpdate_ = 0;
  }

  void Map::update( const GameTime time )
  {
    if ( time > nextCreepUpdate_ )
    {
      updateCreep();
      updateZergBuildable();
      nextCreepUpdate_ = time + g_CVar_creep_updateinterval.as_i();
    }
  }

  void Map::updateReservedMap()
  {
    reservedMap_.reset( Reserved_None );
    creepTumors_.clear();

    auto applyFootprint = [this]( const Point2DI& pt, UnitTypeID ut )
    {
      auto& dbUnit = Database::unit( ut );

      Point2DI topleft(
        pt.x + dbUnit.footprintOffset.x,
        pt.y + dbUnit.footprintOffset.y
      );

      if ( topleft.x < 0 || topleft.y < 0
        || ( topleft.x + dbUnit.footprint.width() ) >= width_
        || ( topleft.y + dbUnit.footprint.height() ) >= height_ )
        return;

      for ( size_t y = 0; y < dbUnit.footprint.height(); y++ )
        for ( size_t x = 0; x < dbUnit.footprint.width(); x++ )
          if ( dbUnit.footprint[x][y] == UnitData::Footprint_Reserved )
          {
            auto& pixel = zergBuildable_[topleft.x + x][topleft.y + y];
            pixel = ( pixel > CreepTile_No ? CreepTile_Walkable : CreepTile_No );
            reservedMap_[topleft.x + x][topleft.y + y] = Reserved_Reserved;
          }
          else if ( dbUnit.footprint[x][y] == UnitData::Footprint_NearResource && reservedMap_[topleft.x + x][topleft.y + y] != Reserved_Reserved )
          {
            reservedMap_[topleft.x + x][topleft.y + y] = Reserved_NearResource;
          }
    };

    // get blocking units (i.e. structures)
    auto blockingUnits = bot_->observation().GetUnits( []( const Unit& unit ) -> bool
    {
      if ( !unit.is_alive )
        return false;

      if ( unit.unit_type == UNIT_TYPEID::ZERG_CREEPTUMOR || unit.unit_type == UNIT_TYPEID::ZERG_CREEPTUMORBURROWED )
        return true;

      auto& dbUnit = Database::unit( unit.unit_type );
      return ( dbUnit.structure && !dbUnit.flying && !dbUnit.footprint.empty() );
    } );

    // loop through structures and subtract footprints from otherwise buildable area
    for ( auto unit : blockingUnits )
    {
      if ( unit->unit_type == UNIT_TYPEID::ZERG_CREEPTUMOR || unit->unit_type == UNIT_TYPEID::ZERG_CREEPTUMORBURROWED )
        creepTumors_.emplace_back( math::floor( unit->pos.x ), math::floor( unit->pos.y ) );

      Point2DI pos(
        math::floor( unit->pos.x ),
        math::floor( unit->pos.y )
      );

      applyFootprint( pos, unit->unit_type );
    }

    // loop through reservations (footprints of where we're going to build)
    for ( auto& r : buildingReservations_ )
    {
      applyFootprint( r.second.position_, r.second.type_ );
    }
  }

  Creep* Map::creep( size_t x, size_t y )
  {
    if ( !isValid( x, y ) )
      return nullptr;

    auto label = labeledCreeps_[x][y];
    if ( label < 1 )
      return nullptr;

    for ( auto& creep : creeps_ )
      if ( creep.label == label )
        return &creep;

    return nullptr;
  }

  Region* Map::region( size_t x, size_t y )
  {
    if ( !isValid( x, y ) )
      return nullptr;

    auto id = regionMap_[x][y];
    if ( id < 0 || id >= regions_.size() )
      return nullptr;

    return regions_[id];
  }

  bool Map::updateZergBuildable()
  {
    zergBuildable_.reset( CreepTile_No );

    // first mark all areas with creep as passable
    for ( size_t x = 0; x < width_; x++ )
      for ( size_t y = 0; y < height_; y++ )
      {
        if ( !( flagsMap_[x][y] & MapFlag_Walkable ) )
          continue;
        if ( !creepMap_[x][y] )
          continue;
        if ( !( flagsMap_[x][y] & MapFlag_Buildable ) )
          zergBuildable_[x][y] = CreepTile_Walkable;
        else
          zergBuildable_[x][y] = CreepTile_Buildable;
      }

    updateReservedMap();

    // contour & label creep bitmap
    labelBuildableCreeps();

    // split to fronts
    splitCreepFronts();

    return true;
  }

  bool Map::canZergBuild( UnitTypeID structure, size_t x, size_t y, int padding, bool nearResources, bool noMainOverlap, bool notNearMain, bool notNearRamp )
  {
    auto& dbUnit = Database::unit( structure );

    Point2DI topleft(
      ( (int)x + dbUnit.footprintOffset.x ) - padding,
      ( (int)y + dbUnit.footprintOffset.y ) - padding
    );

    int fullwidth = ( (int)dbUnit.footprint.width() + ( padding * 2 ) );
    int fullheight = ( (int)dbUnit.footprint.height() + ( padding * 2 ) );

    if ( topleft.x < 0 || topleft.y < 0 || ( topleft.x + fullwidth >= width_ ) || ( topleft.y + fullheight >= height_ ) )
      return false;

    for ( int j = 0; j < fullheight; j++ )
      for ( int i = 0; i < fullwidth; i++ )
      {
        auto mx = ( topleft.x + i );
        auto my = ( topleft.y + j );

        if ( i < padding || j < padding || i >= ( fullwidth - padding ) || j >= ( fullheight - padding ) )
        {
          if ( reservedMap_[mx][my] == Reserved_Reserved || ( !nearResources && reservedMap_[mx][my] == Reserved_NearResource ) )
            return false;
          continue;
        }

        if ( dbUnit.footprint[j - padding][i - padding] == UnitData::Footprint_Reserved )
        {
          if ( zergBuildable_[mx][my] != CreepTile_Buildable )
            return false;
          if ( noMainOverlap && ( flagsMap_[mx][my] & MapFlag_StartLocation ) )
            return false;
          if ( notNearMain && ( flagsMap_[mx][my] & MapFlag_NearStartLocation ) )
            return false;
          if ( notNearRamp && ( flagsMap_[mx][my] & MapFlag_NearRamp ) )
            return false;
        }
      }

    return true;
  }

  bool Map::isBuildable( const MapPoint2& position, UnitTypeID structure, bool nearResources )
  {
    auto& dbUnit = Database::unit( structure );

    Point2DI topleft(
      position.x + dbUnit.footprintOffset.x,
      position.y + dbUnit.footprintOffset.y
    );

    int fullwidth = (int)dbUnit.footprint.width();
    int fullheight = (int)dbUnit.footprint.height();

    if ( topleft.x < 0 || topleft.y < 0 || ( topleft.x + fullwidth >= width_ ) || ( topleft.y + fullheight >= height_ ) )
      return false;

    for ( int y = 0; y < fullheight; y++ )
      for ( int x = 0; x < fullwidth; x++ )
      {
        auto mx = ( topleft.x + x );
        auto my = ( topleft.y + y );

        if ( dbUnit.footprint[x][y] == UnitData::Footprint_Reserved )
        {
          if ( !( flagsMap_[mx][my] & MapFlag_Buildable ) || reservedMap_[mx][my] == Reserved_Reserved || ( !nearResources && reservedMap_[mx][my] == Reserved_NearResource ) )
            return false;
        }
      }

    return true;
  }

  bool Map::findClosestBuildablePosition( MapPoint2& position, UnitTypeID structure, bool nearResources )
  {
    if ( isBuildable( position, structure, nearResources ) )
      return true;

    auto& closestTiles = getDistanceMap( position ).sortedTiles();

    for ( auto& tile : closestTiles )
    {
      MapPoint2 pt( tile );
      if ( isBuildable( pt, structure, nearResources ) )
      {
        position = pt;
        return true;
      }
    }

    return false;
  }

  bool Map::rampHasCreepTumor( int x, int y )
  {
    // this would be a lot less dumb and expensive if we ran contour/labeling on the ramps at startup
    for ( auto& tumor : creepTumors_ )
      if ( flagsMap_[x][y] & MapFlag_Ramp )
        if ( math::manhattan( tumor.x, tumor.y, x, y ) < 10 )
          return true;

    return false;
  }

  void Map::splitCreepFronts()
  {
    // tile is valid for a creeping front if:
    // - it's at least 5 tiles away from map edges
    // - it's at least 2 tiles away from any (map) non-walkable tile
    // - it's not dynamically blocked, i.e. have a structure in the way
    // - it's not inside any base resource box
    // - ...or is part of a ramp which has no creep tumor
    auto fnCheckTile = [&]( int x, int y ) -> bool
    {
      if ( x < 5 || y < 5 || x >= ( width_ - 5 ) || y >= ( height_ - 5 ) )
        return false;
      if ( reservedMap_[x][y] == Reserved_Reserved )
        return false;
      if ( flagsMap_[x][y] & MapFlag_ResourceBox )
        return false;
      if ( flagsMap_[x][y] & MapFlag_Ramp && !rampHasCreepTumor( x, y ) )
        return true;
      return ( flagsMap_[x][y] & MapFlag_InnerWalkable );
    };

    for ( auto& creep : creeps_ )
    {
      creep.fronts.clear();
      vector<MapPoint2> front;
      for ( auto& tile : creep.contour )
      {
        if ( fnCheckTile( tile.x, tile.y ) && front.size() < 15 )
          front.push_back( tile );
        else
        {
          if ( !front.empty() )
            creep.fronts.push_back( front );
          front.clear();
        }
      }
    }
  }

  void Map::labelBuildableCreeps()
  {
    int16_t lbl_w = (int16_t)width_;
    int16_t lbl_h = (int16_t)height_;
    blob_algo::label_t* lbl_out = nullptr;
    blob_algo::blob_t* blob_out = nullptr;

    int blob_count = 0;
    for ( size_t y = 0; y < height_; y++ )
      for ( size_t x = 0; x < width_; x++ )
      {
        auto idx = ( y * width_ ) + x;
        contourTraceImageBuffer_[idx] = ( zergBuildable_[x][y] != CreepTile_No ? 0xFF : 0x00 );
      }

    if ( blob_algo::find_blobs( 0, 0, (int16_t)width_, (int16_t)height_, contourTraceImageBuffer_, (int16_t)width_, (int16_t)height_, &lbl_out, &lbl_w, &lbl_h, &blob_out, &blob_count, 0 ) != 1 )
      HIVE_EXCEPT( "find_blobs failed" );

    if ( lbl_w != width_ || lbl_h != height_ )
      HIVE_EXCEPT( "find_blobs returned changed label array dimensions" );

    for ( size_t y = 0; y < lbl_h; y++ )
      for ( size_t x = 0; x < lbl_w; x++ )
      {
        auto idx = ( y * lbl_w ) + x;
        labeledCreeps_[x][y] = (int)lbl_out[idx];
      }

    auto fnConvertContour = []( blob_algo::contour_t& in, Contour& out )
    {
      for ( int i = 0; i < in.count; i++ )
        out.emplace_back( in.points[i * sizeof( int16_t )], in.points[( i * sizeof( int16_t ) ) + 1] );
    };

    creeps_.clear();

    // TODO creep always has label (index+1), could use that
    for ( int i = 0; i < blob_count; i++ )
    {
      Creep creep;
      creep.label = blob_out[i].label;
      fnConvertContour( blob_out[i].external, creep.contour );
      creeps_.push_back( creep );
    }

    ::free( lbl_out );

    blob_algo::destroy_blobs( blob_out, blob_count );
  }

  BaseLocation* Map::closestLocation( const Vector2 & position )
  {
    Real bestDist = std::numeric_limits<Real>::max();
    BaseLocation* best = nullptr;
    for ( auto& loc : baseLocations_ )
    {
      if ( loc.containsPosition( position ) )
        return &loc;
      auto dist = loc.position().squaredDistance( position );
      if ( dist < bestDist )
      {
        bestDist = dist;
        best = &loc;
      }
    }
    return best;
  }

  Vector2 Map::closestByGround( const Vector2 & from, const list<Vector2> & to )
  {
    if ( to.empty() )
      return from;

    Real bestDistance = std::numeric_limits<Real>::max();
    Vector2 retval;
    for ( auto& loc : to )
    {
      auto dist = bot_->query().PathingDistance( from, loc );
      if ( dist < bestDistance )
      {
        bestDistance = dist;
        retval = loc;
      }
    }
    return retval;
  }

  /*Path Map::shortestScoutPath( const Vector2& start, vector<Vector2>& locations )
  {
    Path retval;
    list<Vector2> loclist( locations.begin(), locations.end() );
    std::function<void( const Vector2&, Path&, list<Vector2>& )> recurse = [&]( const Vector2& pos, Path& out, list<Vector2>& in )
    {
      Vector2 closest = closestByGround( pos, in );
      out.push_back( closest );
      in.remove( closest ); // somewhat dangerous float == float comparison for value removal
      if ( !in.empty() )
        recurse( closest, out, in );
    };
    recurse( start, retval, loclist );
    return retval;
  }*/

  inline uint64_t util_encodePoint( const Point2DI& pt )
  {
    return ( ( (uint64_t)pt.x ) << 32 ) | ( (uint64_t)pt.y );
  };

  Chokepoint* Map::chokepoint( ChokepointID index )
  {
    if ( index < 0 || index >= chokepoints_.size() )
      return nullptr;

    return &chokepoints_[index];
  }

  void Map::reserveFootprint( const Point2DI& position, UnitTypeID type )
  {
    buildingReservations_[util_encodePoint( position )] = BuildingReservation( position, type );
  }

  void Map::clearFootprint( const Point2DI& position )
  {
    buildingReservations_.erase( util_encodePoint( position ) );
  }

  bool Map::isValid( size_t x, size_t y ) const
  {
    return ( x >= 0 && y >= 0 && x < width_ && y < height_ );
  }

  bool Map::isPowered( const Vector2& position ) const
  {
    for ( auto& powerSource : bot_->observation().GetPowerSources() )
      if ( position.distance( powerSource.position ) < powerSource.radius )
        return true;

    return false;
  }

  bool Map::isExplored( const Vector2& position ) const
  {
    if ( !isValid( position ) )
      return false;

    Visibility vis = bot_->observation().GetVisibility( position );
    return ( vis == Visibility::Fogged || vis == Visibility::Visible );
  }

  bool Map::isVisible( const Vector2& position ) const
  {
    if ( !isValid( position ) )
      return false;

    return ( bot_->observation().GetVisibility( position ) == Visibility::Visible );
  }

  bool Map::isWalkable( size_t x, size_t y )
  {
    if ( !isValid( x, y ) )
      return false;

    return ( flagsMap_[x][y] & MapFlag_Walkable );
  }

  bool Map::isBuildable( size_t x, size_t y )
  {
    if ( !isValid( x, y ) )
      return false;

    return ( flagsMap_[x][y] & MapFlag_Buildable );
  }

  const DistanceMap & Map::getDistanceMap( const Point2D & tile ) const
  {
    std::pair<size_t, size_t> index( (size_t)tile.x, (size_t)tile.y );

    if ( distanceMapCache_.find( index ) == distanceMapCache_.end() )
    {
      distanceMapCache_[index] = DistanceMap();
      distanceMapCache_[index].compute( bot_, tile );
    }

    return distanceMapCache_[index];
  }

}