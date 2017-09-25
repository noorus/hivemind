#pragma once
#include "hive_types.h"
#include "hive_math.h"
#include <sc2api/sc2_common.h>

namespace hivemind {

  struct Vector2 {
  public:
    Real x, y;
  public:
    //! Constructors
    inline Vector2(): x( 0.0f ), y( 0.0f ) {}
    inline Vector2( const Real _x, const Real _y ): x( _x ), y( _y ) {}
    inline explicit Vector2( const Real scalar ): x( scalar ), y( scalar ) {}
    inline explicit Vector2( const Real coords[2] ): x( coords[0] ), y( coords[1] ) {}
    inline explicit Vector2( const int coords[2] )
    {
      x = (Real)coords[0];
      y = (Real)coords[1];
    }
    inline explicit Vector2( Real* const coords ): x( coords[0] ), y( coords[1] ) {}
    inline Vector2( const sc2::Point2D& pt ): x( pt.x ), y( pt.y ) {}
    inline Vector2( const sc2::Point3D& pt ): x( pt.x ), y( pt.y ) {}
    //! Swap
    inline void swap( Vector2& other )
    {
      std::swap( x, other.x );
      std::swap( y, other.y );
    }
    //! Accessors
    inline Real operator [] ( const size_t i ) const { return *( &x + i ); }
    inline Real& operator [] ( const size_t i ) { return *( &x + i ); }
    inline Real* ptr() { return &x; }
    inline const Real* ptr() const { return &x; }
    //! Assignment
    inline Vector2& operator = ( const Vector2& rhs ) = default;
    inline Vector2& operator = ( const Real scalar )
    {
      x = scalar;
      y = scalar;
      return *this;
    }
    //! Equality
    inline bool operator == ( const Vector2& rhs ) const
    {
      return ( x == rhs.x && y == rhs.y );
    }
    inline bool operator != ( const Vector2& rhs ) const
    {
      return ( x != rhs.x && y != rhs.y );
    }
    inline bool operator < ( const Vector2& rhs ) const
    {
      return ( x < rhs.x && y < rhs.y );
    }
    inline bool operator > ( const Vector2& rhs ) const
    {
      return ( x > rhs.x && y > rhs.y );
    }
    inline bool operator <= ( const Vector2& rhs ) const
    {
      return ( x <= rhs.x && y <= rhs.y );
    }
    inline bool operator >= ( const Vector2& rhs ) const
    {
      return ( x >= rhs.x && y >= rhs.y );
    }
    //! Arithmetic
    inline Vector2 operator + ( const Vector2& rhs ) const
    {
      return Vector2( x + rhs.x, y + rhs.y );
    }
    inline Vector2 operator - ( const Vector2& rhs ) const
    {
      return Vector2( x - rhs.x, y - rhs.y );
    }
    inline Vector2 operator * ( const Vector2& rhs ) const
    {
      return Vector2( x * rhs.x, y * rhs.y );
    }
    inline Vector2 operator * ( const Real scalar ) const
    {
      return Vector2( x * scalar, y * scalar );
    }
    inline Vector2 operator / ( const Vector2& rhs ) const
    {
      return Vector2( x / rhs.x, y / rhs.y );
    }
    inline Vector2 operator / ( const Real scalar ) const
    {
      Real invert = ( 1.0f / scalar );
      return Vector2( x * invert, y * invert );
    }
    inline const Vector2& operator + () const
    {
      return *this;
    }
    inline Vector2& operator - () const
    {
      return Vector2( -x, -y );
    }
    //! Scalar helpers
    inline friend Vector2 operator * ( const Real scalar, const Vector2& vec )
    {
      return Vector2( scalar * vec.x, scalar * vec.y );
    }
    inline friend Vector2 operator / ( const Real scalar, const Vector2& vec )
    {
      return Vector2( scalar / vec.x, scalar / vec.y );
    }
    inline friend Vector2 operator + ( const Vector2& lhs, const Real rhs )
    {
      return Vector2( lhs.x + rhs, lhs.y + rhs );
    }
    inline friend Vector2 operator + ( const Real lhs, const Vector2& rhs )
    {
      return Vector2( lhs + rhs.x, lhs + rhs.y );
    }
    inline friend Vector2 operator - ( const Vector2& lhs, const Real rhs )
    {
      return Vector2( lhs.x - rhs, lhs.y - rhs );
    }
    inline friend Vector2 operator - ( const Real lhs, const Vector2& rhs )
    {
      return Vector2( lhs - rhs.x, lhs - rhs.y );
    }
    //! Arithmetic update
    inline Vector2& operator += ( const Vector2& rhs )
    {
      x += rhs.x;
      y += rhs.y;
      return *this;
    }
    inline Vector2& operator += ( const Real scalar )
    {
      x += scalar;
      y += scalar;
      return *this;
    }
    inline Vector2& operator -= ( const Vector2& rhs )
    {
      x -= rhs.x;
      y -= rhs.y;
      return *this;
    }
    inline Vector2& operator -= ( const Real scalar )
    {
      x -= scalar;
      y -= scalar;
      return *this;
    }
    inline Vector2& operator *= ( const Vector2& rhs )
    {
      x *= rhs.x;
      y *= rhs.y;
      return *this;
    }
    inline Vector2& operator *= ( const Real scalar )
    {
      x *= scalar;
      y *= scalar;
      return *this;
    }
    inline Vector2& operator /= ( const Vector2& rhs )
    {
      x /= rhs.x;
      y /= rhs.y;
      return *this;
    }
    inline Vector2& operator /= ( const Real scalar )
    {
      Real invert = ( 1.0f / scalar );
      x *= invert;
      y *= invert;
      return *this;
    }
    //! Implicit conversions
    inline Vector2& operator = ( const sc2::Point2D& rhs )
    {
      x = rhs.x;
      y = rhs.y;
      return *this;
    }
    inline Vector2& operator = ( const sc2::Point3D& rhs )
    {
      x = rhs.x;
      y = rhs.y;
      return *this;
    }
    inline operator sc2::Point2D() const
    {
      sc2::Point2D ret;
      ret.x = x;
      ret.y = y;
      return ret;
    }
    inline operator sc2::Point3D() const
    {
      sc2::Point3D ret;
      ret.x = x;
      ret.y = y;
      ret.z = 0;
      return ret;
    }
    //! Operations
    inline Real length() const
    {
      return math::sqrt( x * x + y * y );
    }
    inline Real squaredLength() const
    {
      return ( x * x + y * y );
    }
    inline Real distance( const Vector2& rhs ) const
    {
      return ( *this - rhs ).length();
    }
    inline Real squaredDistance( const Vector2& rhs ) const
    {
      return ( *this - rhs ).squaredLength();
    }
    inline Real dotProduct( const Vector2& other ) const
    {
      return ( x * other.x + y * other.y );
    }
    inline Real normalise()
    {
      auto len = length();
      if ( len > 0.0f ) {
        Real invLen = ( 1.0f / len );
        x *= invLen;
        y *= invLen;
      }
      return len;
    }
    inline Vector2 perpendicular() const
    {
      return Vector2( -y, x );
    }
    inline Real crossProduct( const Vector2& other ) const
    {
      return ( x * other.y - y * other.x );
    }
    inline Vector2 reflect( const Vector2& normal ) const
    {
      return Vector2( *this - ( 2 * this->dotProduct( normal ) * normal ) );
    }
    inline Vector2 normalisedCopy() const
    {
      Vector2 ret = *this;
      ret.normalise();
      return ret;
    }
  };

  using Path = vector<Vector2>;

}