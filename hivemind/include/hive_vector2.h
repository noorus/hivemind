#pragma once
#include "hive_types.h"
#include "hive_math.h"
#include <sc2api/sc2_common.h>

namespace hivemind {

  struct Vector3;

  //! \struct Vector2
  //! \brief Two-dimensional geometric (Euclidean) floating point vector.
  //! \sa Vector3
  struct Vector2 {
  public:
    Real x; //!< X-axis- or first component of this vector.
    Real y; //!< Y-axis- or second component of this vector.
  public:
    //! Constructors
    inline Vector2(): x( 0.0f ), y( 0.0f ) {}
    inline Vector2( const Vector2& other ) : x( other.x ), y( other.y ) {}
    inline Vector2( const Real _x, const Real _y ): x( _x ), y( _y ) {}
    inline explicit Vector2( const Real scalar ): x( scalar ), y( scalar ) {}
    inline explicit Vector2( const Real coords[2] ): x( coords[0] ), y( coords[1] ) {}
    inline explicit Vector2( const int coords[2] )
    {
      x = (Real)coords[0];
      y = (Real)coords[1];
    }
    inline Vector2( const sc2::Point2D& pt ): x( pt.x ), y( pt.y ) {}
    inline Vector2( const sc2::Point3D& pt ): x( pt.x ), y( pt.y ) {}
    //! Swap
    inline void swap( Vector2& other )
    {
      std::swap( x, other.x );
      std::swap( y, other.y );
    }
    //! Assignment
    inline Vector2& operator = ( const Vector2& rhs )
    {
      x = rhs.x;
      y = rhs.y;
      return *this;
    }
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
      return !(*this == rhs);
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
    inline Vector2 operator + () const
    {
      return *this;
    }
    inline Vector2 operator - () const
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
    /*inline operator sc2::Point3D() const
    {
      sc2::Point3D ret;
      ret.x = x;
      ret.y = y;
      ret.z = 0.0f;
      return ret;
    }*/
    //! \fn static Vector2 fromAngle( Real rad, Real length = 1.0f )
    //! \brief Make a 2D direction vector from given angle in radians.
    //! \param angle Angle in radians.
    //! \param length Length of vector, defaults to unit length.
    //! \return The direction vector.
    static Vector2 fromAngle( Radian angle, Real length = 1.0f )
    {
      return Vector2(
        length * math::cos( angle ),
        length * math::sin( angle )
      );
    }
    //! \fn Vector3 to3() const
    //! \brief Returns a three-dimensional copy of this vector, with a zero Z component.
    //! \param z Z component value to set, defaults to zero.
    //! \return A Vector3 copy of this vector.
    //! \sa Vector3
    Vector3 to3( const Real z = 0.0f ) const;
    //! \fn inline Real length() const
    //! \brief Gets the length of the vector, seen as distance from zero origin.
    //! \return Scalar length of the vector.
    inline Real length() const
    {
      return math::sqrt( x * x + y * y );
    }
    //! \fn inline Real squaredLength() const
    //! \brief Gets the squared length of the vector for comparative purposes,
    //!   avoiding the costly square root calculation.
    //! \return Scalar squared length of the vector.
    inline Real squaredLength() const
    {
      return ( x * x + y * y );
    }
    //! \fn inline Real distance( const Vector2& rhs ) const
    //! \brief Gets the distance to another vector.
    //! \param rhs The other vector.
    //! \return Scalar distance between the vectors.
    inline Real distance( const Vector2& rhs ) const
    {
      return ( *this - rhs ).length();
    }
    //! \fn inline Real squaredDistance( const Vector2& rhs ) const
    //! \brief Gets the squared distance to another vector for comparative purposes,
    //!   avoiding the costly square root calculation.
    //! \param rhs The other vector.
    //! \return Scalar squared distance between the vectors.
    inline Real squaredDistance( const Vector2& rhs ) const
    {
      return ( *this - rhs ).squaredLength();
    }
    //! \fn inline Real dotProduct( const Vector2& other ) const
    //! \brief Computes the dot product between this and another vector.
    //! \param other The other vector.
    //! \return Scalar dot product between the vectors.
    inline Real dotProduct( const Vector2& other ) const
    {
      return ( x * other.x + y * other.y );
    }
    //! \fn inline void normalise()
    //! \brief Normalises the vector in place, scaling it to unit length while retaining direction.
    //!   Does nothing if vector is zero-length.
    inline void normalise()
    {
      auto len = length();
      if ( len > 0.0f )
      {
        Real invLen = ( 1.0f / len );
        x *= invLen;
        y *= invLen;
      }
    }
    //! \fn inline Vector2 perpendicular() const
    //! \brief Returns a vector perpendicular to this one.
    //! \return A new perpendicular vector.
    inline Vector2 perpendicular() const
    {
      return Vector2( -y, x );
    }
    //! \fn inline Real crossProduct( const Vector2& other ) const
    //! \brief Computes the cross product between this and another vector.
    //! \param other The other vector.
    //! \return Scalar cross product between the vectors.
    inline Real crossProduct( const Vector2& other ) const
    {
      return ( x * other.y - y * other.x );
    }
    //! \fn inline Vector2 reflect( const Vector2& normal ) const
    //! \brief Calculates a reflection vector from this pointing away from given plane normal.
    //! \param normal The normal.
    //! \return A reflected vector.
    inline Vector2 reflect( const Vector2& normal ) const
    {
      return Vector2( *this - ( 2 * this->dotProduct( normal ) * normal ) );
    }
    //! \fn inline Vector2 normalisedCopy() const
    //! \brief Returns a normalised copy of this vector.
    //! \return Normalised copy of this vector.
    //! \sa normalise()
    inline Vector2 normalisedCopy() const
    {
      Vector2 ret = *this;
      ret.normalise();
      return ret;
    }
  };

}