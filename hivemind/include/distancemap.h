#pragma once
#include "hive_vector2.h"
#include "hive_array2.h"

namespace hivemind {

  class Bot;

  class DistanceMap {
  private:
    size_t width_;
    size_t height_;
    Vector2 startTile_;
    Array2<int> dist_;
    vector<Vector2> sorted_;
  public:
    void compute( Bot* bot, const Vector2& startTile );
    int dist( size_t x, size_t y ) const;
    int dist( const Vector2& pos ) const;
    inline const vector<Vector2>&  sortedTiles() const { return sorted_; }
    inline const Vector2& startTile() const { return startTile_; }
  };

}