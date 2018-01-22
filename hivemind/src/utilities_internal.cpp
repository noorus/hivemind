#include "stdafx.h"
#include "utilities.h"
#include "hive_vector2.h"
#include "hive_vector3.h"

namespace hivemind {

  // sorry
  Vector3 Vector2::to3( const Real z ) const
  {
    return Vector3( x, y, z );
  }

  namespace utils {
    namespace internal {

      Real pointDistanceToPoly( const Vector2& pt, const vector<Vector2>& poly )
      {
        Real bestDist = std::numeric_limits<Real>::max();

        if ( poly.size() < 2 )
          return bestDist;

        auto previous = poly.back();
        for ( auto& current : poly )
        {
          auto dist = utils::distanceToLineSegment( previous, current, pt );
          if ( dist < bestDist )
            bestDist = dist;
          previous = current;
        }

        return bestDist;
      }

      int pointInsidePolyOuter( const Vector2& pt, const vector<Vector2>& poly )
      {
        if ( poly.size() < 3 )
          return 0;

        int result = 0;
        auto iter = poly[0];
        for ( size_t i = 1; i <= poly.size(); i++ )
        {
          auto next = ( i == poly.size() ? poly[0] : poly[i] );
          if ( next.y == pt.y )
          {
            if ( next.x == pt.x || ( iter.y == pt.y && ( next.x > pt.x == iter.x < pt.x ) ) )
              return -1;
          }
          if ( iter.y < pt.y != next.y < pt.y )
          {
            if ( iter.x >= pt.x )
            {
              if ( next.x > pt.x )
                result = 1 - result;
              else
              {
                auto d = ( iter.x - pt.x ) * ( next.y - pt.y ) - ( next.x - pt.x ) * ( iter.y - pt.y );
                if ( d == 0.0f )
                  return -1;
                if ( d > 0 == next.y > iter.y )
                  result = 1 - result;
              }
            }
            else
            {
              if ( next.x > pt.x )
              {
                auto d = ( iter.x - pt.x ) * ( next.y - pt.y ) - ( next.x - pt.x ) * ( iter.y - pt.y );
                if ( d == 0.0f )
                  return -1;
                if ( d > 0 == next.y > iter.y )
                  result = 1 - result;
              }
            }
          }
          iter = next;
        }

        return result;
      }

    }
  }
}