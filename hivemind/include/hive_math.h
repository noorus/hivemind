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

    template <typename T>
    inline T abs( T value )
    {
      return ::abs( value );
    }

    inline Real clamp( Real value, Real low, Real high )
    {
      return ( value < low ? low : ( value > high ? high : value ) );
    }

  }

}