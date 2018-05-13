#pragma once
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"

namespace hivemind {

  class Bot;

  class DistanceMap {
  private:
    size_t width_;
    size_t height_;
    MapPoint2 startTile_;
    Array2<int> dist_;
    vector<MapPoint2> sorted_;
  public:
    void compute( Bot* bot, const MapPoint2& startTile );
    int dist( size_t x, size_t y ) const;
    inline int dist( const MapPoint2& pos ) const { return dist( pos.x, pos.y ); }
    inline int dist( const Vector2& pos ) const { return dist( (size_t)pos.x, (size_t)pos.y ); }
    inline const vector<MapPoint2>& sortedTiles() const { return sorted_; }
    inline const MapPoint2& startTile() const { return startTile_; }
  };

}