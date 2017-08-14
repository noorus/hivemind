#include "stdafx.h"
#include "map.h"
#include "bot.h"
#include "utilities.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

namespace hivemind {

  const size_t c_legalActions = 4;
  const int c_actionX[c_legalActions] = { 1, -1, 0, 0 };
  const int c_actionY[c_legalActions] = { 0, 0, 1, -1 };

  Map::Map( Bot* bot ): bot_( bot ), width_( 0 ), height_( 0 )
  {
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

  void Map::rebuild()
  {
    bot_->console().printf( "Map: Rebuilding..." );

    width_ = bot_->observation().GetGameInfo().width;
    height_ = bot_->observation().GetGameInfo().height;

    bot_->console().printf( "Map: Width %d, height %d", width_, height_ );

    flagsMap_.resize( width_, height_ );
    heightMap_.resize( width_, height_ );

    auto gameInfo = &bot_->observation().GetGameInfo();

    for ( size_t x = 0; x < width_; x++ )
      for ( size_t y = 0; y < height_; y++ )
      {
        uint64_t flags = 0;
        if ( utils::placement( *gameInfo, x, y ) )
          flags |= MapFlag_Buildable;
        if ( ( flags & MapFlag_Buildable ) || utils::pathable( *gameInfo, x, y ) )
          flags |= MapFlag_Walkable;

        flagsMap_[x][y] = flags;

        heightMap_[x][y] = utils::terrainHeight( *gameInfo, x, y );
      }

    bot_->console().printf( "Map: Got build-, walkability- and height map" );

    debugDumpMaps( flagsMap_, heightMap_, *gameInfo );

    bot_->console().printf( "Map: Rebuild done" );
  }

  void Map::draw()
  {
    Point2D camera = bot_->observation().GetCameraPos();
    for ( float x = camera.x - 16.0f; x < camera.x + 16.0f; ++x )
    {
      for ( float y = camera.y - 16.0f; y < camera.y + 16.0f; ++y )
      {
        if ( !isValid( (size_t)x, (size_t)y ) )
          continue;
        Point3D pt;
        pt.x = x;
        pt.y = y;
        pt.z = heightMap_[(size_t)x][(size_t)y] + 0.25f;
        sc2::Color clr = sc2::Colors::Red;
        if ( flagsMap_[(size_t)x][(size_t)y] & MapFlag_Buildable )
          clr = sc2::Colors::Green;
        else if ( flagsMap_[(size_t)x][(size_t)y] & MapFlag_Walkable )
          clr = sc2::Colors::Yellow;
        bot_->debug().DebugSphereOut( pt, 0.25f, clr );
      }
    }
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

}