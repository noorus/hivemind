#pragma once
#include "sc2_forward.h"

namespace hivemind {

  namespace utils {

    constexpr GameTime timeToTicks( const uint16_t minutes, const uint16_t seconds )
    {
      const float speed_multiplier = 1.4f; // faster
      const GameTime ticks_per_second = 16; // 32 updates per second / 2 updates per tick
      return (GameTime)( ( ( minutes * 60 ) + seconds ) * ticks_per_second * speed_multiplier );
    }

    inline const RealTime ticksToTime( const GameTime ticks )
    {
      const float speed_multiplier = 1.4f; // faster
      const float ticks_per_second = 16.0f; // 32 updates per second / 2 updates per tick
      return (RealTime)roundf( ( (float)ticks / speed_multiplier ) / ticks_per_second );
    }

  }

}