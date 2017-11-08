#pragma once
#include "hive_types.h"

namespace hivemind {

  namespace math {

    //! \fn inline Real round( Real value )
    //! \brief Rounds given floating point value.
    //! \param value A value.
    //! \return Rounded value.
    inline Real round( Real value )
    {
      return ::roundf( value );
    }

    //! \fn inline Real sqrt( Real value )
    //! \brief Square root of given floating point value.
    //! \param value A value.
    //! \return The square root of value.
    inline Real sqrt( Real value )
    {
      return ::sqrtf( value );
    }

    //! \fn inline int floor( Real value )
    //! \brief Floors the given floating point value and returns an integer,
    //!   due to this being the most common use case.
    //! \param value The value.
    //! \return Given value as floored integer.
    inline int floor( Real value )
    {
      return (int)::floorf( value );
    }

    //! \fn abs
    //! \brief Absolute value of float, double, long etc.
    //! \param value A value.
    //! \return Absolute positive of the value.
    template <typename T>
    inline T abs( T value )
    {
      return ::abs( value );
    }

    //! \fn inline Real clamp( Real value, Real low, Real high )
    //! \brief Clamps the given value between a high bound and a low bound.
    //! \param value The value.
    //! \param low   The low.
    //! \param high  The high.
    //! \return Clamped value.
    inline Real clamp( Real value, Real low, Real high )
    {
      return ( value < low ? low : ( value > high ? high : value ) );
    }

    //! \fn inline T manhattan( T x1, T y1, T x2, T y2 )
    //! \brief 2D Manhattan distance with coordinates of any type supported by abs().
    //! \param x1 The first x value.
    //! \param y1 The first y value.
    //! \param x2 The second x value.
    //! \param y2 The second y value.
    //! \return Manhattan distance between x1,y1 and x2,y2.
    template <typename T>
    inline T manhattan( T x1, T y1, T x2, T y2 )
    {
      return ( abs( x2 - x1 ) + abs( y2 - y1 ) );
    }

  }

}