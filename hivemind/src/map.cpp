#include "stdafx.h"
#include "map.h"
#include "bot.h"
#include "utilities.h"
#include "map_analysis.h"
#include "hive_geometry.h"
#include "baselocation.h"
#include "database.h"
#include "blob_algo.h"

#pragma warning(disable: 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"
#pragma warning(default: 4996)
#include "external/simple_svg_1.0.0.hpp"

namespace hivemind {

  const size_t c_legalActions = 4;
  const int c_actionX[c_legalActions] = { 1, -1, 0, 0 };
  const int c_actionY[c_legalActions] = { 0, 0, 1, -1 };

  Map::Map( Bot* bot ): bot_( bot ), width_( 0 ), height_( 0 ), contourTraceImageBuffer_( nullptr )
  {
  }

  Map::~Map()
  {
    if ( contourTraceImageBuffer_ )
      ::free( contourTraceImageBuffer_ );
  }

  void debugDumpMaps( Array2<uint64_t>& flagmap, Array2<Real>& heightmap, const GameInfo& info )
  {
    Array2<uint8_t> height8( info.width, info.height );
    Array2<uint8_t> build8( info.width, info.height );
    Array2<uint8_t> path8( info.width, info.height );

    for ( size_t y = 0; y < info.height; y++ )
      for ( size_t x = 0; x < info.width; x++ )
      {
        uint8_t val;
        val = (uint8_t)( ( ( heightmap[x][y] + 200.0f ) / 400.0f ) * 255.0f );
        height8[y][x] = val;
        val = ( flagmap[x][y] & MapFlag_Buildable ) ? 0xFF : 0x00;
        build8[y][x] = val;
        val = ( flagmap[x][y] & MapFlag_Walkable ) ? 0xFF : 0x00;
        path8[y][x] = val;
      }

    stbi_write_png( "debug_map_height.png", info.width, info.height, 1, height8.data(), info.width );
    stbi_write_png( "debug_map_buildable.png", info.width, info.height, 1, build8.data(), info.width );
    stbi_write_png( "debug_map_pathable.png", info.width, info.height, 1, path8.data(), info.width );
  }

#pragma pack(push, 1)
  struct rgb {
    uint8_t r, g, b;
  };
#pragma pack(pop)

  void debugDumpLabels( Array2<int>& labels )
  {
    const rgb unwalkable = { 0, 0, 0 };
    const rgb contour = { 255, 255, 255 };

    Array2<rgb> rgb8( labels.width(), labels.height() );

    for ( size_t y = 0; y < labels.height(); y++ )
      for ( size_t x = 0; x < labels.width(); x++ )
      {
        if ( labels[x][y] == -1 )
          rgb8[y][x] = contour;
        else if ( labels[x][y] == 0 )
          rgb8[y][x] = unwalkable;
        else {
          rgb tmp;
          utils::hsl2rgb( ( (uint16_t)labels[x][y] - 1 ) * 120, 230, 200, (uint8_t*)&tmp );
          rgb8[y][x] = tmp;
        }
      }

    stbi_write_png( "debug_map_components.png", (int)labels.width(), (int)labels.height(), 3, rgb8.data(), (int)labels.width() * 3 );
  }

  void debugDumpResources( vector<UnitVector>& clusters, const GameInfo& info )
  {
    const rgb empty = { 0, 0, 0 };
    const rgb field = { 132, 47, 132 };
    const rgb mineral = { 4, 218, 255 };
    const rgb gas = { 129, 255, 15 };

    Array2<rgb> rgb8( info.width, info.height );
    rgb8.reset( empty );

    for ( size_t y = 0; y < info.height; y++ )
      for ( size_t x = 0; x < info.width; x++ )
      {
        Vector2 pos( (Real)x, (Real)y );
        for ( auto& cluster : clusters )
        {
          bool gotit = false;
          for ( auto unit : cluster )
            if ( pos.distance( unit->pos ) <= unit->radius ) {
              rgb8[y][x] = ( utils::isMineral( unit ) ? mineral : gas );
              gotit = true;
              break;
            }
          if ( !gotit && pos.distance( cluster.center() ) <= 14.0f ) // shouldn't be hardcoded
            rgb8[y][x] = field;
        }
      }

    stbi_write_png( "debug_map_clusters.png", (int)info.width, (int)info.height, 3, rgb8.data(), (int)info.width * 3 );
  }

  void dumpPolygons( size_t width, size_t height, PolygonComponentVector& polys, std::map<Analysis::RegionNodeID, Analysis::Chokepoint>& chokes )
  {
    svg::Dimensions dim( (double)width * 16.0, (double)height * 16.0 );
    svg::Document doc( "debug_map_polygons_invert.svg", svg::Layout( dim, svg::Layout::TopLeft ) );
    for ( auto& poly : polys )
    {
      svg::Polygon svgPoly( svg::Stroke( 2.0, svg::Color::Purple ) );

      for ( auto& pt : poly.contour )
        svgPoly << svg::Point( pt.x * 16.0, pt.y * 16.0 );
      for ( auto& hole : poly.holes )
      {
        svg::Polygon holePoly( svg::Stroke( 2.0, svg::Color::Red ) );
        for ( auto& pt : hole )
          holePoly << svg::Point( pt.x * 16.0, pt.y * 16.0 );
        doc << holePoly;
      }
      doc << svgPoly;
    }
    for ( auto& pair : chokes )
    {
      Vector2 p0( pair.second.side1.x, pair.second.side1.y );
      Vector2 p1( pair.second.side2.x, pair.second.side2.y );
      svg::Line line( svg::Point( p0.x * 16, p0.y * 16 ), svg::Point( p1.x * 16, p1.y * 16 ), svg::Stroke( 2.0, svg::Color::Cyan ) );
      doc << line;
    }
    doc.save();
  }

  void Map::rebuild()
  {
    bot_->console().printf( "Map: Rebuilding..." );

    bot_->console().printf( "Map: Clearing distance map cache" );
    distanceMapCache_.clear();

    const GameInfo& info = bot_->observation().GetGameInfo();

    Analysis::Map_BuildBasics( info, width_, height_, flagsMap_, heightMap_ );

    for ( auto unit : bot_->observation().GetUnits( Unit::Alliance::Neutral ) )
      maxZ_ = std::max( unit->pos.z, maxZ_ );

    bot_->console().printf( "Map: Width %d, height %d", width_, height_ );
    bot_->console().printf( "Map: Got build-, walkability- and height map" );

    debugDumpMaps( flagsMap_, heightMap_, info );

    bot_->console().printf( "Map: Initializing dynamic data" );
    creepMap_.resize( width_, height_ );
    creepMap_.reset( false );
    zergBuildable_.resize( width_, height_ );
    zergBuildable_.reset( CreepTile_No );
    labeledCreeps_.resize( width_, height_ );
    labeledCreeps_.reset( -1 );

    if ( contourTraceImageBuffer_ )
      ::free( contourTraceImageBuffer_ );

    contourTraceImageBuffer_ = (uint8_t*)::malloc( width_ * height_ );

    bot_->console().printf( "Map: Processing contours..." );

    components_.clear();
    polygons_.clear();
    obstacles_.clear();

    Analysis::Map_ProcessContours( flagsMap_, labelsMap_, components_ );

    debugDumpLabels( labelsMap_ );

    bot_->console().printf( "Map: Generating polygons..." );

    Analysis::Map_ComponentPolygons( components_, polygons_ );

    //bot_->console().printf( "Map: Inverting walkable polygons to obstacles..." );

    //Analysis::Map_InvertPolygons( polygons_, obstacles_, Rect2( info.playable_min, info.playable_max ), Vector2( (Real)info.width, (Real)info.height ) );
    
    obstacles_ = polygons_;

    bot_->console().printf( "Map: Generating Voronoi diagram..." );

    bgi::rtree<BoostSegmentI, bgi::quadratic<16> > rtree;
    Analysis::Map_MakeVoronoi( info, obstacles_, labelsMap_, graph_, rtree );

    bot_->console().printf( "Map: Pruning Voronoi diagram..." );

    Analysis::Map_PruneVoronoi( graph_ );

    bot_->console().printf( "Map: Detecting nodes..." );

    Analysis::Map_DetectNodes( graph_, obstacles_ );

    bot_->console().printf( "Map: Simplifying graph..." );

    Analysis::Map_SimplifyGraph( graph_, graphSimplified_ );

    bot_->console().printf( "Map: Merging graph region nodes..." );

    Analysis::Map_MergeRegionNodes( graphSimplified_ );

    bot_->console().printf( "Map: Expanding chokepoints..." );

    Analysis::Map_GetChokepointSides( graphSimplified_, rtree, chokepointSides_ );

    dumpPolygons( width_, height_, obstacles_, chokepointSides_ );

    bot_->console().printf( "Map: Finding resource clusters..." );

    resourceClusters_.clear();

    Analysis::Map_FindResourceClusters( bot_->observation(), resourceClusters_, 4, 16.0f );

    debugDumpResources( resourceClusters_, info );

    bot_->console().printf( "Map: Creating base locations..." );

    baseLocations_.clear();

    size_t index = 0;
    for ( auto& cluster : resourceClusters_ )
      baseLocations_.emplace_back( bot_, index++, cluster );

    bot_->console().printf( "Map: Rebuild done" );
  }

  void drawPoly( sc2::DebugInterface& debug, Polygon& poly, Real z, sc2::Color color )
  {
    auto previous = poly.back();
    for ( auto& vec : poly )
    {
      Point3D p0( previous.x + 0.5f, previous.y + 0.5f, z + 0.1f );
      Point3D p1( vec.x + 0.5f, vec.y + 0.5f, z + 0.1f );
      debug.DebugLineOut( p0, p1, color );
      previous = vec;
    }
  }

  void Map::draw()
  {
    for ( auto& poly_comp : polygons_ )
    {
      drawPoly( bot_->debug(), poly_comp.contour, maxZ_, sc2::Colors::Purple );
      for ( auto& hole : poly_comp.holes )
        drawPoly( bot_->debug(), hole, maxZ_, sc2::Colors::Red );
    }
    for ( auto& asd : chokepointSides_ )
    {
      Point3D p0( asd.second.side1.x, asd.second.side1.y, maxZ_ );
      Point3D p1( asd.second.side2.x, asd.second.side2.y, maxZ_ );
      bot_->debug().DebugLineOut( p0, p1, sc2::Colors::Green );
    }
    for ( auto& node : graphSimplified_.nodes )
    {
      bot_->debug().DebugSphereOut( sc2::Point3D( node.x, node.y, maxZ_ ), 0.25f, sc2::Colors::Teal );
    }
    for ( auto& cluster : resourceClusters_ )
    {
      auto pos = cluster.center();
      // bot_->debug().DebugSphereOut( Point3D( pos.x, pos.y, maxZ_ ), 10.0f );
      for ( auto res : cluster )
        bot_->debug().DebugSphereOut( res->pos, 1.0f, sc2::Colors::Yellow );
    }
    for ( auto& location : baseLocations_ )
    {
      char msg[64];
      sprintf_s( msg, 64, "Base location %zd", location.baseID_ );
      bot_->debug().DebugTextOut( msg, Point3D( location.position_.x, location.position_.y, maxZ_ ), sc2::Colors::White );
      Real height = heightMap_[math::floor( location.position_.x )][math::floor( location.position_.y )];
      bot_->debug().DebugBoxOut(
        Point3D( location.left_, location.top_, height ),
        Point3D( location.right_, location.bottom_, height + 1.0f ), sc2::Colors::White );
      bot_->debug().DebugSphereOut( Point3D( location.position_.x, location.position_.y, height ), 10.0f, sc2::Colors::White );
    }
    for ( auto& creep : creeps_ )
    {
      rgb tmp;
      sc2::Color colorContour;
      utils::hsl2rgb( ( (uint16_t)creep.label - 1 ) * 120, 230, 200, (uint8_t*)&tmp );
      colorContour.r = tmp.r;
      colorContour.g = tmp.g;
      colorContour.b = tmp.b;
      for ( auto& pt : creep.contour )
        bot_->debug().DebugSphereOut( sc2::Point3D( (Real)pt.x + 0.5f, (Real)pt.y + 0.5f, heightMap_[pt.x][pt.y] + 0.5f ), 0.1f, colorContour );
    }
    /*sc2::Color buildable;
    buildable.r = 164;
    buildable.g = 0;
    buildable.b = 112;
    for ( size_t x = 0; x < width_; x++ )
      for ( size_t y = 0; y < height_; y++ )
        if ( zergBuildable_[x][y] )
          bot_->debug().DebugSphereOut( sc2::Point3D( (Real)x + 0.5f, (Real)y + 0.5f, heightMap_[x][y] ), 0.1f, buildable );*/
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

    // get blocking units (i.e. structures)
    auto blockingUnits = bot_->observation().GetUnits( []( const Unit& unit ) -> bool {
      if ( !unit.is_alive )
        return false;
      auto& dbUnit = Database::unit( unit.unit_type );
      return ( dbUnit.structure && !dbUnit.flying && !dbUnit.footprint.empty() );
    } );

    // loop through structures and subtract footprints from otherwise buildable area
    for ( auto unit : blockingUnits )
    {
      auto& dbUnit = Database::unit( unit->unit_type );
      Point2DI topleft(
        math::floor( unit->pos.x ) + dbUnit.footprintOffset.x,
        math::floor( unit->pos.y ) + dbUnit.footprintOffset.y
      );
      for ( size_t y = 0; y < dbUnit.footprint.height(); y++ )
        for ( size_t x = 0; x < dbUnit.footprint.width(); x++ )
          if ( dbUnit.footprint[y][x] )
          {
            auto& pixel = zergBuildable_[topleft.x + x][topleft.y + y];
            pixel = ( pixel > CreepTile_No ? CreepTile_Walkable : CreepTile_No );
          }
    }

    // contour & label creep bitmap
    labelBuildableCreeps();

    return true;
  }

  void Map::labelBuildableCreeps()
  {
    int16_t lbl_w = width_;
    int16_t lbl_h = height_;
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
        labeledCreeps_[x][y] = lbl_out[idx];
      }

    auto fnConvertContour = []( blob_algo::contour_t& in, Contour& out )
    {
      for ( int i = 0; i < in.count; i++ )
        out.emplace_back( in.points[i * sizeof( int16_t )], in.points[( i * sizeof( int16_t ) ) + 1] );
    };

    creeps_.clear();

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
    Real bestDist = 30000.0f;
    BaseLocation* best = nullptr;
    for ( auto& loc : baseLocations_ )
    {
      if ( loc.containsPosition( position ) )
        return &loc;
      auto dist = loc.getPosition().squaredDistance( position );
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

    Real bestDistance = 30000.0f;
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

  Path Map::shortestScoutPath( const Vector2& start, vector<Vector2>& locations )
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