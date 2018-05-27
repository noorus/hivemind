#include "stdafx.h"
#include "debug.h"
#include "utilities.h"

#ifdef HIVE_SUPPORT_MAP_DUMPS
#pragma warning(disable: 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"
#pragma warning(default: 4996)
#include "external/simple_svg_1.0.0.hpp"
#endif

namespace hivemind {

  Point3D DebugExtended::mapTileToMarker( const Vector2& v, const Array2<Real>& heightmap, Real offset, Real maxZ )
  {
    MapPoint2 tile = MapPoint2( math::floor( v.x ), math::floor( v.y ) );
    maxZ += offset;
    Real z = heightmap[tile.x][tile.y] + offset;
    if ( z > ( maxZ + offset ) )
      z = ( maxZ + offset );
    return Point3D( v.x + 0.5f, v.y + 0.5f, z );
  }

  Real mapTileHeight( const Vector2& v, const Array2<Real>& heightmap, Real offset )
  {
    MapPoint2 tile = MapPoint2( math::floor( v.x ), math::floor( v.y ) );
    return heightmap[tile.x][tile.y] + offset;
  }

  void DebugExtended::drawMapPolygon( Map& map, const Polygon& poly, Color color, const Vector2& offset )
  {
    auto previous = poly.back();
    for ( auto& vec : poly )
    {
      Point3D p0 = mapTileToMarker( previous + offset, map.heightMap_, 0.4f, map.maxZ_ );
      Point3D p1 = mapTileToMarker( vec + offset, map.heightMap_, 0.4f, map.maxZ_ );
      drawLine( p0, p1, color );
      drawSphere( p1, 0.1f, color );
      previous = vec;
    }
  }

  void DebugExtended::drawMapBarePolygon( Map& map, const Polygon& poly, Color color, const Vector2& offset, Real offsetZ )
  {
    auto previous = poly.back();
    for ( auto& vec : poly )
    {
      auto tmpPrev = previous + offset;
      auto tmpCur = vec + offset;
      Point3D p0( tmpPrev.x, tmpPrev.y, mapTileHeight( tmpPrev, map.heightMap_, offsetZ ) );
      Point3D p1( tmpCur.x, tmpCur.y, mapTileHeight( tmpPrev, map.heightMap_, offsetZ ) );
      drawLine( p0, p1, color );
      drawSphere( p1, 0.1f, color );
      previous = vec;
    }
  }

  void DebugExtended::drawMapLine( Map& map, const Vector2& p0, const Vector2& p1, Color color )
  {
    Point3D p30 = mapTileToMarker( p0, map.heightMap_, 0.5f, map.maxZ_ );
    Point3D p31 = mapTileToMarker( p1, map.heightMap_, 0.5f, map.maxZ_ );
    drawLine( p30, p31, color );
  }

  void DebugExtended::drawMapSphere( Map & map, const Vector2 & pos, Real radius, Color color )
  {
    Point3D pt = mapTileToMarker( pos, map.heightMap_, 0.5f, map.maxZ_ );
    drawSphere( pt, radius, color );
  }

#ifdef HIVE_SUPPORT_MAP_DUMPS

  void DebugExtended::mapDumpBasicMaps( Array2<uint64_t>& flagmap, Array2<Real>& heightmap, const GameInfo& info )
  {
    Array2<uint8_t> height8( info.height, info.width );
    Array2<uint8_t> build8( info.height, info.width );
    Array2<uint8_t> path8( info.height, info.width );
    Array2<uint8_t> block8( info.height, info.width );

    for ( size_t x = 0; x < info.width; x++ )
      for ( size_t y = 0; y < info.height; y++ )
      {
        size_t row = info.height - y - 1;
        size_t column = x;

        uint8_t h = (uint8_t)( ( ( heightmap[x][y] + 200.0f ) / 400.0f ) * 255.0f );
        height8[row][column] = h;

        uint8_t b = ( flagmap[x][y] & MapFlag_Buildable ) ? 0xFF : 0x00;
        build8[row][column] = b;

        uint8_t p = ( flagmap[x][y] & MapFlag_Walkable ) ? 0xFF : 0x00;
        path8[row][column] = p;

        uint8_t blocked = ( flagmap[x][y] & MapFlag_Blocker ) ? 0xFF : 0x00;
        block8[row][column] = blocked;
      }

    stbi_write_png( "debug_map_height.png", info.width, info.height, 1, height8.data(), info.width );
    stbi_write_png( "debug_map_flags_buildable.png", info.width, info.height, 1, build8.data(), info.width );
    stbi_write_png( "debug_map_flags_pathable.png", info.width, info.height, 1, path8.data(), info.width );
    stbi_write_png( "debug_map_flags_blockers.png", info.width, info.height, 1, block8.data(), info.width );
  }

  void DebugExtended::mapDumpLabelMap( Array2<int>& map, bool contoured, const string& name )
  {
    const rgb background = { 0, 0, 0 };
    const rgb contour = { 255, 255, 255 };

    Array2<rgb> rgb8( map.height(), map.width() );

    for ( size_t x = 0; x < map.width(); x++ )
      for ( size_t y = 0; y < map.height(); y++ )
      {
        size_t row = map.height() - y - 1;
        size_t column = x;

        rgb color;
        if ( contoured )
        {
          if ( map[x][y] == -1 )
            color = contour;
          else if ( map[x][y] == 0 )
            color = background;
          else
          {
            utils::hsl2rgb( ( (uint16_t)map[x][y] - 1 ) * 120, 230, 200, (uint8_t*)&color );
          }
        }
        else
        {
          utils::hsl2rgb( ( (uint16_t)map[x][y] ) * 120, 230, 200, (uint8_t*)&color );
        }

        rgb8[row][column] = color;
      }

    char filename[64];
    sprintf_s( filename, 64, "debug_map_%s.png", name.c_str() );
    stbi_write_png( filename, (int)map.width(), (int)map.height(), 3, rgb8.data(), (int)map.width() * 3 );
  }

  void DebugExtended::mapDumpBaseLocations( Array2<uint64_t>& flagmap, vector<UnitVector>& clusters, const GameInfo& info, BaseLocationVector& bases )
  {
    const rgb empty = { 0, 0, 0 };
    const rgb field = { 132, 47, 132 };
    const rgb mineral = { 4, 218, 255 };
    const rgb gas = { 129, 255, 15 };
    const rgb startloc = { 228, 255, 0 };
    const rgb startpoint = { 255, 0, 0 };

    Array2<rgb> rgb8( info.height, info.width );

    for ( size_t x = 0; x < info.width; x++ )
      for ( size_t y = 0; y < info.height; y++ )
      {
        size_t row = info.height - y - 1;
        size_t column = x;

        rgb color = empty;

        Vector2 pos( (Real)x, (Real)y );
        for ( auto& cluster : clusters )
        {
          bool gotit = false;
          if ( flagmap[x][y] & MapFlag_StartLocation )
          {
            color = startloc;
            gotit = true;
          }
          if ( !gotit )
            for ( auto unit : cluster )
              if ( pos.distance( unit->pos ) <= unit->radius )
              {
                color = ( utils::isMineral( unit ) ? mineral : gas );
                gotit = true;
                break;
              }
          if ( !gotit && pos.distance( cluster.center() ) <= 14.0f ) // shouldn't be hardcoded
            color = field;
        }

        rgb8[row][column] = color;
      }

    for ( auto& base : bases )
    {
      size_t x = (int)base.position().x;
      size_t y = (int)base.position().y;

      size_t row = info.height - y - 1;
      size_t column = x;

      rgb8[row][column] = startpoint;
    }

    stbi_write_png( "debug_map_bases.png", (int)info.width, (int)info.height, 3, rgb8.data(), (int)info.width * 3 );
  }

  const double cVectorImageSizeMultiplier = 32.0;

  void DebugExtended::mapDumpPolygons( size_t width, size_t height, PolygonComponentVector& polys, RegionVector& regions, ChokeVector& chokes, const Point2Vector& startLocations )
  {
    svg::Dimensions dim( (double)width * cVectorImageSizeMultiplier, (double)height * cVectorImageSizeMultiplier );
    svg::Document doc( "debug_map_polygons.svg", svg::Layout( dim, svg::Layout::BottomLeft ) );
    /*for ( auto& poly : polys )
    {
      svg::Polygon svgPoly( svg::Stroke( 1.0, svg::Color( 50, 50, 50 ) ) );

      for ( auto& pt : poly.contour )
        svgPoly << svg::Point( pt.x * 16.0, pt.y * 16.0 );
      for ( auto& hole : poly.contour.holes )
      {
        svg::Polygon holePoly( svg::Stroke( 1.0, svg::Color( 50, 50, 50 ) ) );
        for ( auto& pt : hole )
          holePoly << svg::Point( pt.x * 16.0, pt.y * 16.0 );
        doc << holePoly;
      }
      doc << svgPoly;
    }*/
    for ( auto& region : regions )
    {
      if ( region->dubious_ )
        continue;
      auto clr = utils::prettyColor( region->label_, 25 );
      svg::Polygon svgPoly( svg::Stroke( 4.0, svg::Color( clr.r, clr.g, clr.b ) ) );
      for ( auto& vec : region->polygon_ )
      {
        svgPoly << svg::Point( vec.x * cVectorImageSizeMultiplier, vec.y * cVectorImageSizeMultiplier );
      }
      doc << svgPoly;
    }
    for ( auto& choke : chokes )
    {
      svg::Line line(
        svg::Point( choke.side1.x * cVectorImageSizeMultiplier, choke.side1.y * cVectorImageSizeMultiplier ),
        svg::Point( choke.side2.x * cVectorImageSizeMultiplier, choke.side2.y * cVectorImageSizeMultiplier ),
        svg::Stroke( 6.0, svg::Color::Green ) );
      doc << line;
    }
    for ( auto& loc : startLocations )
    {
      auto pt = svg::Point( (double)loc.x * cVectorImageSizeMultiplier, (double)loc.y * cVectorImageSizeMultiplier );
      svg::Circle circle( pt, 5.0 * cVectorImageSizeMultiplier, svg::Fill( svg::Color::Red ) );
      doc << circle;
    }
    /*for ( size_t id = 0; id < graph.adjacencyList_.size(); id++ )
    {
      for ( auto adj : graph.adjacencyList_[id] )
      {
        auto v0 = graph.nodes_[id];
        auto v1 = graph.nodes_[adj];
        svg::Line line(
          svg::Point( v0.x * cVectorImageSizeMultiplier, v0.y * cVectorImageSizeMultiplier ),
          svg::Point( v1.x * cVectorImageSizeMultiplier, v1.y * cVectorImageSizeMultiplier ),
          svg::Stroke( 6.0, svg::Color::Cyan ) );
        doc << line;
      }
    }*/
    doc.save();
  }

#endif

}