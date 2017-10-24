#include "stdafx.h"
#include "bot.h"
#include "baselocation.h"
#include "utilities.h"

namespace hivemind {

  const int c_nearBaseLocationTileDistance = 20;

  BaseLocation::BaseLocation( Bot* bot, size_t baseID, const UnitVector& resources ):
  bot_( bot ), baseID_( baseID ), startLocation_( false ),
  left_( std::numeric_limits<Real>::max() ), right_( std::numeric_limits<Real>::lowest() ),
  top_( std::numeric_limits<Real>::lowest() ), bottom_( std::numeric_limits<Real>::max() )
  {
    Point2D resourceCenter( 0.0f, 0.0f );

    for ( auto resource : resources )
    {
      if ( utils::isMineral( resource ) )
        minerals_.push_back( resource );
      else
        geysers_.push_back( resource );
      resourceCenter += resource->pos;
      Real resWidth = 1.0f;
      Real resHeight = 0.5f;
      left_ = std::min( left_, resource->pos.x - resWidth );
      right_ = std::max( right_, resource->pos.x + resWidth );
      top_ = std::max( top_, resource->pos.y + resHeight );
      bottom_ = std::min( bottom_, resource->pos.y - resHeight );
    }

    auto numResources = minerals_.size() + geysers_.size();
    position_ = Point2D( left_ + ( right_ - left_ ) / 2.0f, top_ + ( bottom_ - top_ ) / 2.0f );
    distanceMap_ = bot_->map().getDistanceMap( position_ );

    for ( auto& pos : bot_->observation().GetGameInfo().enemy_start_locations )
      if ( containsPosition( pos ) )
      {
        startLocation_ = true;
        position_ = pos;
        break;
      }

    for ( auto unit : bot_->observation().GetUnits() )
      if ( utils::isMine( unit ) && utils::isMainStructure( unit ) && containsPosition( unit->pos ) )
      {
        startLocation_ = true;
        position_ = unit->pos;
        break;
      }
  }

  int BaseLocation::getGroundDistance( const Vector2 & pos ) const
  {
    return distanceMap_.dist( pos );
  }

  bool BaseLocation::isStartLocation() const
  {
    return startLocation_;
  }

  bool BaseLocation::containsPosition( const Vector2 & pos ) const
  {
    if ( !bot_->map().isValid( pos ) || ( pos.x == 0 && pos.y == 0 ) )
      return false;

    return ( getGroundDistance( pos ) < c_nearBaseLocationTileDistance );
  }

  bool BaseLocation::isInResourceBox( int x, int y ) const
  {
    return ( x >= left_ && x < right_ && y < top_ && y >= bottom_ );
  }

  const vector<Vector2>& BaseLocation::getClosestTiles() const
  {
    return distanceMap_.sortedTiles();
  }

}