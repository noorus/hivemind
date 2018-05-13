#include "stdafx.h"
#include "distancemap.h"
#include "bot.h"

namespace hivemind {

  const size_t c_legalActions = 4;
  const int c_actionX[c_legalActions] = { 1, -1, 0, 0 };
  const int c_actionY[c_legalActions] = { 0, 0, 1, -1 };

  int DistanceMap::dist( size_t x, size_t y ) const
  {
    return dist_[x][y];
  }

  void DistanceMap::compute( Bot* bot, const MapPoint2& startTile )
  {
    startTile_ = startTile;

    width_ = bot->map().width_;
    height_ = bot->map().height_;

    dist_.resize( width_, height_ );
    dist_.reset( -1 );

    sorted_.reserve( width_ * height_ );

    vector<MapPoint2> fringe;
    fringe.reserve( width_ * height_ );
    fringe.push_back( startTile );
    sorted_.push_back( startTile );

    dist_[startTile.x][startTile.y] = 0;

    for ( size_t fringeIndex = 0; fringeIndex < fringe.size(); ++fringeIndex )
    {
      const MapPoint2& tile = fringe[fringeIndex];
      for ( size_t a = 0; a < c_legalActions; ++a )
      {
        MapPoint2 nextTile( tile.x + c_actionX[a], tile.y + c_actionY[a] );
        if ( bot->map().isWalkable( nextTile.x, nextTile.y ) && dist( nextTile ) == -1 )
        {
          dist_[nextTile.x][nextTile.y] = dist_[tile.x][tile.y] + 1;
          fringe.push_back( nextTile );
          sorted_.push_back( nextTile );
        }
      }
    }
  }

}