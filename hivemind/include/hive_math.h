#pragma once
#include "hive_types.h"
#include "sc2_forward.h"
#include "hive_vector2.h"

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