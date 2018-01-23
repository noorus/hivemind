#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"

namespace hivemind {

  namespace Analysis {

    struct Chokepoint {
      Vector2 side1;
      Vector2 side2;
      Chokepoint( Vector2 s1, Vector2 s2 ): side1( s1 ), side2( s2 ) {}
      inline const Vector2 middle() const
      {
        return ( side1 + ( ( side1 - side2 ) * 0.5f ) );
      }
    };

  }

}