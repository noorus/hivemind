#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "chokepoint.h"
#include "pathing.h"

namespace hivemind {

  class Region;

  using RegionSet = set<Region*>;
  using RegionVector = vector<Region*>;

  struct RegionNode
  {
  public:
    Vector2 position_;
    RegionNode( const Vector2& position )
        : position_( position ) {}
  };

  using RegionNodeVector = vector<RegionNode>;

  class Region
  {
  public:
    int label_;
    Polygon polygon_;
    Real opennessDistance_;
    Vector2 opennessPoint_;
    Real height_;
    int tileCount_;
    int heightLevel_; //!< AKA cliff level or whatever, 0 being lowest on map
    bool dubious_; //!< If this region has no real area or tiles, and probably lacks known height value
    ChokeSet chokepoints_;
    RegionSet reachableRegions_;
    RegionNodeVector nodes_;
    map<pair<ChokepointID, ChokepointID>, CachedPath> chokePaths_;
    const CachedPath& getChokePath( ChokepointID from, ChokepointID to ) const;
  };
}