#pragma once
#include "hive_types.h"

namespace hivemind {

  namespace Math {

    inline Real round( Real value )
    {
      return ::roundf( value );
    }

    inline Real sqrt( Real value )
    {
      return ::sqrtf( value );
    }

    inline Real abs( Real value )
    {
      return ::fabsf( value );
    }

    inline Real clamp( Real value, Real low, Real high )
    {
      return ( value < low ? low : ( value > high ? high : value ) );
    }

  }

}