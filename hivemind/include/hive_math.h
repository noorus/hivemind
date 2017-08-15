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

  }

}