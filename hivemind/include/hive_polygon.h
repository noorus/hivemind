#pragma once
#include "hive_types.h"
#include "hive_vector2.h"
#include "hive_math.h"
#include "hive_geometry.h"

namespace hivemind {

  struct MapPoint2 {
    int x;
    int y;
    MapPoint2( int x_, int y_ ): x( x_ ), y( y_ ) {}
  };

  using Contour = vector<MapPoint2>;
  using ContourVector = vector<Contour>;

  class Polygon;

  using PolygonVector = vector<Polygon>;

  class Polygon: public vector<Vector2> {
  public:
    PolygonVector holes;
  public:
    const Real area() const
    {
      if ( this->size() < 3 )
        return 0.0f;

      Real a = this->back().x * this->front().y - this->front().x * this->back().y;
      size_t lastPoint = this->size() - 1;
      for ( size_t i = 0, j = 1; i < lastPoint; ++i, ++j )
      {
        a += this->at( i ).x * this->at( j ).y - this->at( j ).x * this->at( i ).y;
      }
      return math::abs( a / 2.0f );
    }
  };

  inline Polygon util_clipperPathToPolygon( const ClipperLib::Path& in )
  {
    Polygon out;
    for ( auto& pt : in )
    {
      Real x = (Real)pt.X / 1000.0f;
      Real y = (Real)pt.Y / 1000.0f;
      out.emplace_back( x, y );
    }
    return out;
  }

  inline ClipperLib::Path util_polyToClipperPath( const Polygon& in )
  {
    ClipperLib::Path out;
    for ( auto& pt : in )
    {
      auto x = ( ClipperLib::cInt )( pt.x * 1000.0f );
      auto y = ( ClipperLib::cInt )( pt.y * 1000.0f );
      out.emplace_back( x, y );
    }
    return out;
  }

  inline ClipperLib::Path util_contourToClipperPath( const Contour& in )
  {
    ClipperLib::Path out;
    for ( auto& pt : in )
    {
      auto x = ( ClipperLib::cInt )( pt.x * 1000 );
      auto y = ( ClipperLib::cInt )( pt.y * 1000 );
      out.emplace_back( x, y );
    }
    return out;
  }

  inline BoostPolygon util_clipperPathToBoostPolygon( const ClipperLib::Path& in )
  {
    BoostPolygon out;
    for ( auto &pt : in )
    {
      auto x = (double)pt.X / 1000.0;
      auto y = (double)pt.Y / 1000.0;
      boost::geometry::append( out, BoostPoint( x, y ) );
    }
    return out;
  }

  inline Polygon util_boostPolygonToPolygon( BoostPolygon& in )
  {
    Polygon out;
    boost::geometry::for_each_point( in, [&]( BoostPoint const& pt )
    {
      out.emplace_back( (Real)pt.x(), (Real)pt.y() );
    } );
    return out;
  }

  inline ClipperLib::Path util_boostPolygonToClipperPath( const BoostPolygon& in )
  {
    ClipperLib::Path out;
    boost::geometry::for_each_point( in, [&]( BoostPoint const& pt )
    {
      auto x = ( ClipperLib::cInt )( pt.x() * 1000.0f );
      auto y = ( ClipperLib::cInt )( pt.y() * 1000.0f );
      out.emplace_back( x, y );
    } );
    return out;
  }

  inline ClipperLib::Paths util_boostMultiPolyToClipperPaths( const BoostMultiPoly& polys )
  {
    ClipperLib::Paths outPolys;
    for ( auto& poly : polys )
    {
      auto clipPoly = util_boostPolygonToClipperPath( poly );
      outPolys.push_back( clipPoly );
    }
    return outPolys;
  }

}