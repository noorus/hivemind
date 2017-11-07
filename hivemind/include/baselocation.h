#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "distancemap.h"
#include "hive_rect2.h"

namespace hivemind {

  class Bot;

  struct BaseLocation {
  public:
    Bot* bot_;
    DistanceMap distanceMap_;
    UnitVector geysers_;
    UnitVector minerals_;
    Vector2 position_;
    size_t baseID_;
    Real left_;
    Real right_;
    Real top_;
    Real bottom_;
    bool startLocation_;
  public:
    BaseLocation( Bot* bot, size_t baseID, const UnitVector& resources );
    int getGroundDistance( const Vector2& pos ) const;
    bool isStartLocation() const;
    bool containsPosition( const Vector2& pos ) const;
    const Vector2& position() const { return position_; }
    const bool hasMinerals() const { return !minerals_.empty(); }
    const bool hasGeysers() const { return !geysers_.empty(); }
    const UnitVector& getGeysers() const { return minerals_; }
    const UnitVector& getMinerals() const { return geysers_; }
    bool isInResourceBox( int x, int y ) const;
    bool overlapsMainFootprint( int x, int y ) const;
    const vector<Vector2>& getClosestTiles() const;
  };

  using BaseLocationVector = vector<BaseLocation>;

}