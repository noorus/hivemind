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

  void DebugExtended::drawMapPolygon( Map& map, Polygon& poly, Color color )
  {
    auto previous = poly.back();
    for ( auto& vec : poly )
    {
      Point3D p0 = mapTileToMarker( previous, map.heightMap_, 0.5f, map.maxZ_ );
      Point3D p1 = mapTileToMarker( vec, map.heightMap_, 0.5f, map.maxZ_ );
      drawLine( p0, p1, color );
      drawSphere( p1, 0.1f, color );
      previous = vec;
    }
  }

#ifdef HIVE_SUPPORT_MAP_DUMPS

  void DebugExtended::mapDumpBasicMaps( Array2<uint64_t>& flagmap, Array2<Real>& heightmap, const GameInfo& info )
  {
    Array2<uint8_t> height8( info.width, info.height );
    Array2<uint8_t> build8( info.width, info.height );
    Array2<uint8_t> path8( info.width, info.height );

    for ( size_t y = 0; y < info.height; y++ )
      for ( size_t x = 0; x < info.width; x++ )
      {
        uint8_t val;
        val = (uint8_t)( ( ( heightmap[x][y] + 200.0f ) / 400.0f ) * 255.0f );
        height8[y][x] = val;
        val = ( flagmap[x][y] & MapFlag_Buildable ) ? 0xFF : 0x00;
        build8[y][x] = val;
        val = ( flagmap[x][y] & MapFlag_Walkable ) ? 0xFF : 0x00;
        path8[y][x] = val;
      }

    stbi_write_png( "debug_map_height.png", info.width, info.height, 1, height8.data(), info.width );
    stbi_write_png( "debug_map_buildable.png", info.width, info.height, 1, build8.data(), info.width );
    stbi_write_png( "debug_map_pathable.png", info.width, info.height, 1, path8.data(), info.width );
  }

  void DebugExtended::mapDumpLabelMap( Array2<int>& map, bool contoured, const string& name )
  {
    const rgb background = { 0, 0, 0 };
    const rgb contour = { 255, 255, 255 };

    Array2<rgb> rgb8( map.width(), map.height() );

    for ( size_t y = 0; y < map.height(); y++ )
      for ( size_t x = 0; x < map.width(); x++ )
      {
        if ( contoured )
        {
          if ( map[x][y] == -1 )
            rgb8[y][x] = contour;
          else if ( map[x][y] == 0 )
            rgb8[y][x] = background;
          else
          {
            rgb tmp;
            utils::hsl2rgb( ( (uint16_t)map[x][y] - 1 ) * 120, 230, 200, (uint8_t*)&tmp );
            rgb8[y][x] = tmp;
          }
        } else
        {
          rgb tmp;
          utils::hsl2rgb( ( (uint16_t)map[x][y] ) * 120, 230, 200, (uint8_t*)&tmp );
          rgb8[y][x] = tmp;
        }
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

    Array2<rgb> rgb8( info.width, info.height );
    rgb8.reset( empty );

    for ( size_t y = 0; y < info.height; y++ )
      for ( size_t x = 0; x < info.width; x++ )
      {
        Vector2 pos( (Real)x, (Real)y );
        for ( auto& cluster : clusters )
        {
          bool gotit = false;
          if ( flagmap[x][y] & MapFlag_StartLocation )
          {
            rgb8[y][x] = startloc;
            gotit = true;
          }
          if ( !gotit )
            for ( auto unit : cluster )
              if ( pos.distance( unit->pos ) <= unit->radius )
              {
                rgb8[y][x] = ( utils::isMineral( unit ) ? mineral : gas );
                gotit = true;
                break;
              }
          if ( !gotit && pos.distance( cluster.center() ) <= 14.0f ) // shouldn't be hardcoded
            rgb8[y][x] = field;
        }
      }

    for ( auto& base : bases )
    {
      rgb8[(int)base.position().y][(int)base.position().x] = startpoint;
    }

    stbi_write_png( "debug_map_bases.png", (int)info.width, (int)info.height, 3, rgb8.data(), (int)info.width * 3 );
  }

  void DebugExtended::mapDumpPolygons( size_t width, size_t height, PolygonComponentVector& polys, std::map<Analysis::RegionNodeID, Analysis::ChokeSides>& chokes )
  {
    svg::Dimensions dim( (double)width * 16.0, (double)height * 16.0 );
    svg::Document doc( "debug_map_polygons_invert.svg", svg::Layout( dim, svg::Layout::TopLeft ) );
    for ( auto& poly : polys )
    {
      svg::Polygon svgPoly( svg::Stroke( 2.0, svg::Color::Purple ) );

      for ( auto& pt : poly.contour )
        svgPoly << svg::Point( pt.x * 16.0, pt.y * 16.0 );
      for ( auto& hole : poly.contour.holes )
      {
        svg::Polygon holePoly( svg::Stroke( 2.0, svg::Color::Red ) );
        for ( auto& pt : hole )
          holePoly << svg::Point( pt.x * 16.0, pt.y * 16.0 );
        doc << holePoly;
      }
      doc << svgPoly;
    }
    for ( auto& pair : chokes )
    {
      Vector2 p0( pair.second.side1.x, pair.second.side1.y );
      Vector2 p1( pair.second.side2.x, pair.second.side2.y );
      svg::Line line( svg::Point( p0.x * 16, p0.y * 16 ), svg::Point( p1.x * 16, p1.y * 16 ), svg::Stroke( 2.0, svg::Color::Cyan ) );
      doc << line;
    }
    doc.save();
  }

#endif

}