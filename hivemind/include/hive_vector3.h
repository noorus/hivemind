#pragma once
#include "hive_types.h"
#include "sc2_forward.h"
#include "hive_math.h"

namespace hivemind {

  //! \struct Vector3
  //! \brief Three-dimensional geometric (Euclidean) floating point vector.
  //! \sa Vector2
  struct Vector3 {
  public:
    Real x; //!< X-axis- or first component of this vector.
    Real y; //!< Y-axis- or second component of this vector.
    Real z; //!< Z-axis- or third component of this vector.
  public:
    //! Constructors
    inline Vector3(): x( 0.0f ), y( 0.0f ), z( 0.0f ) {}
    inline Vector3( const Vector3& other ) : x( other.x ), y( other.y ), z( other.z ) {}
    inline Vector3( const Real _x, const Real _y, const Real _z ) : x( _x ), y( _y ), z( _z ) {}
    inline explicit Vector3( const Real scalar ): x( scalar ), y( scalar ), z( scalar ) {}
    inline explicit Vector3( const Real coords[3] ): x( coords[0] ), y( coords[1] ), z( coords[2] ) {}
    inline explicit Vector3( const int coords[3] )
    {
      x = (Real)coords[0];
      y = (Real)coords[1];
      z = (Real)coords[2];
    }
    inline Vector3( const Point3D& pt ) : x( pt.x ), y( pt.y ), z( pt.z ) {}
    //! Swap
    inline void swap( Vector3& other )
    {
      std::swap( x, other.x );
      std::swap( y, other.y );
      std::swap( z, other.z );
    }
    //! Assignment
    inline Vector3& operator = ( const Vector3& rhs )
    {
      x = rhs.x;
      y = rhs.y;
      z = rhs.z;
      return *this;
    }
    inline Vector3& operator = ( const Real scalar )
    {
      x = scalar;
      y = scalar;
      z = scalar;
      return *this;
    }
    //! Equality
    inline bool operator == ( const Vector3& rhs ) const
    {
      return ( x == rhs.x && y == rhs.y && z == rhs.z );
    }
    inline bool operator != ( const Vector3& rhs ) const
    {
      return !(*this == rhs);
    }
    //! Arithmetic
    inline Vector3 operator + ( const Vector3& rhs ) const
    {
      return Vector3( x + rhs.x, y + rhs.y, z + rhs.z );
    }
    inline Vector3 operator - ( const Vector3& rhs ) const
    {
      return Vector3( x - rhs.x, y - rhs.y, z - rhs.z );
    }
    inline Vector3 operator * ( const Vector3& rhs ) const
    {
      return Vector3( x * rhs.x, y * rhs.y, z * rhs.z );
    }
    inline Vector3 operator * ( const Real scalar ) const
    {
      return Vector3( x * scalar, y * scalar, z * scalar );
    }
    inline Vector3 operator / ( const Vector3& rhs ) const
    {
      return Vector3( x / rhs.x, y / rhs.y, z / rhs.z );
    }
    inline Vector3 operator / ( const Real scalar ) const
    {
      Real invert = ( 1.0f / scalar );
      return Vector3( x * invert, y * invert, z * invert );
    }
    inline Vector3 operator + () const
    {
      return *this;
    }
    inline Vector3 operator - () const
    {
      return Vector3( -x, -y, -z );
    }
    //! Scalar helpers
    inline friend Vector3 operator * ( const Real scalar, const Vector3& vec )
    {
      return Vector3( scalar * vec.x, scalar * vec.y, scalar * vec.z );
    }
    inline friend Vector3 operator / ( const Real scalar, const Vector3& vec )
    {
      return Vector3( scalar / vec.x, scalar / vec.y, scalar / vec.z );
    }
    inline friend Vector3 operator + ( const Vector3& lhs, const Real rhs )
    {
      return Vector3( lhs.x + rhs, lhs.y + rhs, lhs.z + rhs );
    }
    inline friend Vector3 operator + ( const Real lhs, const Vector3& rhs )
    {
      return Vector3( lhs + rhs.x, lhs + rhs.y, lhs + rhs.z );
    }
    inline friend Vector3 operator - ( const Vector3& lhs, const Real rhs )
    {
      return Vector3( lhs.x - rhs, lhs.y - rhs, lhs.z - rhs );
    }
    inline friend Vector3 operator - ( const Real lhs, const Vector3& rhs )
    {
      return Vector3( lhs - rhs.x, lhs - rhs.y, lhs - rhs.z );
    }
    //! Arithmetic update
    inline Vector3& operator += ( const Vector3& rhs )
    {
      x += rhs.x;
      y += rhs.y;
      z += rhs.z;
      return *this;
    }
    inline Vector3& operator += ( const Real scalar )
    {
      x += scalar;
      y += scalar;
      z += scalar;
      return *this;
    }
    inline Vector3& operator -= ( const Vector3& rhs )
    {
      x -= rhs.x;
      y -= rhs.y;
      z -= rhs.z;
      return *this;
    }
    inline Vector3& operator -= ( const Real scalar )
    {
      x -= scalar;
      y -= scalar;
      z -= scalar;
      return *this;
    }
    inline Vector3& operator *= ( const Vector3& rhs )
    {
      x *= rhs.x;
      y *= rhs.y;
      z *= rhs.z;
      return *this;
    }
    inline Vector3& operator *= ( const Real scalar )
    {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      return *this;
    }
    inline Vector3& operator /= ( const Vector3& rhs )
    {
      x /= rhs.x;
      y /= rhs.y;
      z /= rhs.z;
      return *this;
    }
    inline Vector3& operator /= ( const Real scalar )
    {
      Real invert = ( 1.0f / scalar );
      x *= invert;
      y *= invert;
      z *= invert;
      return *this;
    }
    //! Implicit conversions
    inline Vector3& operator = ( const Point3D& rhs )
    {
      x = rhs.x;
      y = rhs.y;
      z = rhs.z;
      return *this;
    }
    inline operator Point3D() const
    {
      Point3D ret;
      ret.x = x;
      ret.y = y;
      ret.z = z;
      return ret;
    }
    //! \fn inline Vector2 to2() const
    //! \brief Returns a two-dimensional copy of this vector, discarding the Z component.
    //! \return A Vector2 copy of this vector.
    //! \sa Vector2
    inline Vector2 to2() const
    {
      return Vector2( x, y );
    }
    //! \fn inline Real length() const
    //! \brief Gets the length of the vector, seen as distance from zero origin.
    //! \return Scalar length of the vector.
    inline Real length() const
    {
      return math::sqrt( x * x + y * y + z * z );
    }
    //! \fn inline Real squaredLength() const
    //! \brief Gets the squared length of the vector for comparative purposes,
    //!   avoiding the costly square root calculation.
    //! \return Scalar squared length of the vector.
    inline Real squaredLength() const
    {
      return ( x * x + y * y + z * z );
    }
    //! \fn inline Real distance( const Vector3& rhs ) const
    //! \brief Gets the distance to another vector.
    //! \param rhs The other vector.
    //! \return Scalar distance between the vectors.
    inline Real distance( const Vector3& rhs ) const
    {
      return ( *this - rhs ).length();
    }
    //! \fn inline Real squaredDistance( const Vector3& rhs ) const
    //! \brief Gets the squared distance to another vector for comparative purposes,
    //!   avoiding the costly square root calculation.
    //! \param rhs The other vector.
    //! \return Scalar squared distance between the vectors.
    inline Real squaredDistance( const Vector3& rhs ) const
    {
      return ( *this - rhs ).squaredLength();
    }
    //! \fn inline Real dotProduct( const Vector3& other ) const
    //! \brief Computes the dot product between this and another vector.
    //! \param other The other vector.
    //! \return Scalar dot product between the vectors.
    inline Real dotProduct( const Vector3& other ) const
    {
      return ( x * other.x + y * other.y + z * other.z );
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
        z *= invLen;
      }
    }
    //! \fn inline Vector3 crossProduct( const Vector3& other ) const
    //! \brief Computes the cross product between this and another vector.
    //! \param other The other vector.
    //! \return Scalar cross product between the vectors.
    inline Vector3 crossProduct( const Vector3& other ) const
    {
      return Vector3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
      );
    }
    //! \fn inline Vector3 reflect( const Vector3& normal ) const
    //! \brief Calculates a reflection vector from this pointing away from given plane normal.
    //! \param normal The normal.
    //! \return A reflected vector.
    inline Vector3 reflect( const Vector3& normal ) const
    {
      return Vector3( *this - ( 2 * this->dotProduct( normal ) * normal ) );
    }
    //! \fn inline Vector3 normalisedCopy() const
    //! \brief Returns a normalised copy of this vector.
    //! \return Normalised copy of this vector.
    //! \sa normalise()
    inline Vector3 normalisedCopy() const
    {
      Vector3 ret = *this;
      ret.normalise();
      return ret;
    }
    //! \fn inline Vector3 midPoint( const Vector3& other ) const
    //! \brief Returns the middle point between this and another vector.
    //! \param other The other vector.
    //! \return A middle point vector.
    inline Vector3 midPoint( const Vector3& other ) const
    {
      return Vector3(
        ( x + other.x ) * 0.5f,
        ( y + other.y ) * 0.5f,
        ( z + other.z ) * 0.5f
      );
    }
  };

}