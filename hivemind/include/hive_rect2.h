#pragma once
#include "hive_vector2.h"

namespace hivemind {

  struct Rect2 {
    Vector2 min_;
    Vector2 max_;
    Rect2( const Vector2& min, const Vector2& max ): min_( min ), max_( max ) {}
  };

}