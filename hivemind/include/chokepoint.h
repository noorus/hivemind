#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"

namespace hivemind {

  class Region;

  using ChokepointID = int;

  struct Chokepoint {
    ChokepointID id_;
    Vector2 side1;
    Vector2 side2;
    Region* region1;
    Region* region2;
    Chokepoint(): id_( -1 ), region1( nullptr ), region2( nullptr ) {}
    Chokepoint( const ChokepointID id, Vector2 s1, Vector2 s2, Region* r1 = nullptr, Region* r2 = nullptr ): id_( id ), side1( s1 ), side2( s2 ), region1( r1 ), region2( r2 ) {}
    inline const Vector2 middle() const
    {
      return ( side1 + ( ( side2 - side1 ) * 0.5f ) );
    }
  };

  using ChokeVector = vector<Chokepoint>;
  using ChokeSet = set<ChokepointID>;

}