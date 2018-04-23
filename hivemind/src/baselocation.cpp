#include "stdafx.h"
#include "bot.h"
#include "baselocation.h"
#include "utilities.h"
#include "database.h"

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
      if ( initialContainsPosition( pos ) )
      {
        startLocation_ = true;
        position_ = pos;
        break;
      }

    for ( auto unit : bot_->observation().GetUnits() )
      if ( utils::isMine( unit ) && utils::isMainStructure( unit ) && initialContainsPosition( unit->pos ) )
      {
        startLocation_ = true;
        position_ = unit->pos;
        break;
      }

    if ( !startLocation_ )
    {
      MapPoint2 fixpos = position_;
      bot_->map().findClosestBuildablePosition( fixpos, sc2::UNIT_TYPEID::ZERG_HATCHERY, false );
      position_ = static_cast<Vector2>( fixpos );
    }

    auto region = bot_->map().region( position_ );
    if ( !region )
      HIVE_EXCEPT( "Base location is not inside a region" );

    region_ = region->label_;
  }

  int BaseLocation::getGroundDistance( const Vector2& pos ) const
  {
    return distanceMap_.dist( pos );
  }

  bool BaseLocation::isStartLocation() const
  {
    return startLocation_;
  }

  bool BaseLocation::initialContainsPosition( const Vector2& pos ) const
  {
    if ( !bot_->map().isValid( pos ) || ( pos.x == 0.0f && pos.y == 0.0f ) )
      return false;

    return ( getGroundDistance( pos ) < c_nearBaseLocationTileDistance );
  }

  bool BaseLocation::containsPosition( const Vector2& pos ) const
  {
    if ( !bot_->map().isValid( pos ) || ( pos.x == 0.0f && pos.y == 0.0f ) )
      return false;

    return ( bot_->map().closestRegionId( pos ) == region_ );
  }

  bool BaseLocation::isInResourceBox( int x, int y ) const
  {
    return ( x >= left_ && x < right_ && y < top_ && y >= bottom_ );
  }

  bool BaseLocation::overlapsMainFootprint( int x, int y ) const
  {
    // quick early out if distance is not even close to us
    if ( math::manhattan( x, y, (int)position_.x, (int)position_.y ) > 10 )
      return false;

    if ( x < 0 || y < 0 || x >= bot_->map().width() || y >= bot_->map().height() )
      return false;

    auto& dbUnit = Database::unit( (UnitType64)sc2::UNIT_TYPEID::ZERG_HATCHERY );

    MapPoint2 topleft(
      math::floor( position_.x ) + dbUnit.footprintOffset.x,
      math::floor( position_.y ) + dbUnit.footprintOffset.y
    );

    MapPoint2 pt( x - topleft.x, y - topleft.y );
    if ( pt.x < 0 || pt.y < 0 || pt.x >= dbUnit.footprint.width() || pt.y >= dbUnit.footprint.height() )
      return false;

    if ( topleft.x < 0 || topleft.y < 0
      || topleft.x + dbUnit.footprint.width() >= bot_->map().width()
      || topleft.y + dbUnit.footprint.height() >= bot_->map().height() )
      return false; // or?

    return ( dbUnit.footprint[pt.y][pt.x] == UnitData::Footprint_Reserved );
  }

  const vector<Vector2>& BaseLocation::getClosestTiles() const
  {
    return distanceMap_.sortedTiles();
  }

}