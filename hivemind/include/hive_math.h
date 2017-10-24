#pragma once
#include "hive_types.h"

namespace hivemind {

  namespace math {

    inline Real round( Real value )
    {
      return ::roundf( value );
    }

    inline Real sqrt( Real value )
    {
      return ::sqrtf( value );
    }

    inline int floor( Real value )
    {
      return (int)::floorf( value );
    }

    template <typename T>
    inline T abs( T value )
    {
      return ::abs( value );
    }

    inline Real clamp( Real value, Real low, Real high )
    {
      return ( value < low ? low : ( value > high ? high : value ) );
    }

    //! Manhattan distance
    template <typename T>
    inline T manhattan( T x1, T y1, T x2, T y2 )
    {
      return ( abs( x2 - x1 ) + abs( y2 - y1 ) );
    }

  }

}