#pragma once
#include "hive_types.h"
#include "hive_vector2.h"
#include "hive_math.h"

namespace hivemind {

  class Polygon: public vector<Vector2> {
  public:
    const Real area() const
    {
      if ( this->size() < 3 )
        return 0.0f;

      Real a = this->back().x * this->front().y - this->front().x * this->back().y;
      size_t lastPoint = this->size() - 1;
      for ( size_t i = 0, j = 1; i < lastPoint; ++i, ++j )
      {
        a += this->at( i ).x * this->at( j ).y - this->at( j ).x * this->at( i ).y;
      }
      return math::abs( a / 2.0f );
    }
  };

  using PolygonVector = vector<Polygon>;

}