#include "stdafx.h"
#include "map.h"
#include "bot.h"
#include "utilities.h"

namespace hivemind {

  const size_t c_legalActions = 4;
  const int c_actionX[c_legalActions] = { 1, -1, 0, 0 };
  const int c_actionY[c_legalActions] = { 0, 0, 1, -1 };

  Map::Map( Bot* bot ): bot_( bot ), width_( 0 ), height_( 0 )
  {
  }

  void dump( Array2<uint64_t>& flagmap )
  {
    std::ofstream outBuildable( "debug_map_buildable.log" );
    std::ofstream outPathable( "debug_map_pathable.log" );

    for ( size_t y = 0; y < flagmap.height(); y++ )
    {
      for ( size_t x = 0; x < flagmap.width(); x++ )
      {
        outBuildable << ( ( flagmap.column( x )[y] & MapFlag_Buildable ) ? 'x' : '.' );
        outPathable << ( ( flagmap.column( x )[y] & MapFlag_Walkable ) ? 'x' : '.' );
      }
      outBuildable << std::endl;
      outPathable << std::endl;
    }
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

    bot_->console().printf( "Map: Rebuild done" );

    dump( flagsMap_ );
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