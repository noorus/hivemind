#include "stdafx.h"
#include "map.h"
#include "map_analysis.h"
#include "utilities.h"
#include "blob_algo.h"
#include "exception.h"

namespace hivemind {

  namespace Analysis {

    /*
     * 1) Extract from gameinfo: dimensions, buildability, pathability, heightmap
     **/
    void Map_BuildBasics( const GameInfo & info, size_t & width_out, size_t & height_out, Array2<uint64_t>& flags_out, Array2<Real>& heightmap_out )
    {
      width_out = info.width;
      height_out = info.height;

      flags_out.resize( width_out, height_out );
      heightmap_out.resize( width_out, height_out );

      for ( size_t x = 0; x < width_out; x++ )
        for ( size_t y = 0; y < height_out; y++ )
        {
          uint64_t flags = 0;
          if ( utils::placement( info, x, y ) )
            flags |= MapFlag_Buildable;
          if ( ( flags & MapFlag_Buildable ) || utils::pathable( info, x, y ) )
            flags |= MapFlag_Walkable;

          flags_out[x][y] = flags;

          heightmap_out[x][y] = utils::terrainHeight( info, x, y );
        }
    }

    /*
     * 2) Extract from pathability map: contours and connected components
     **/
    void Map_ProcessContours( Array2<uint64_t> flags_in, Array2<int>& labels_out, ComponentVector & components_out )
    {
      auto width = (int16_t)flags_in.width();
      auto height = (int16_t)flags_in.height();

      auto image_input = (uint8_t*)malloc( width * height );

      int16_t lbl_w = width;
      int16_t lbl_h = height;
      blob_algo::label_t* lbl_out = nullptr;
      blob_algo::blob_t* blob_out = nullptr;

      int blob_count = 0;
      for ( size_t y = 0; y < height; y++ )
        for ( size_t x = 0; x < width; x++ )
        {
          auto idx = ( y * width ) + x;
          image_input[idx] = ( ( flags_in[x][y] & MapFlag_Walkable ) ? 0xFF : 0x00 );
        }

      if ( blob_algo::find_blobs( 0, 0, width, height, image_input, width, height, &lbl_out, &lbl_w, &lbl_h, &blob_out, &blob_count, 1 ) != 1 )
        ENGINE_EXCEPT( "find_blobs failed" );

      labels_out.resize( lbl_w, lbl_h );

      for ( size_t y = 0; y < lbl_h; y++ )
        for ( size_t x = 0; x < lbl_w; x++ )
        {
          auto idx = ( y * lbl_w ) + x;
          labels_out[x][y] = lbl_out[idx];
        }

      auto fnConvertContour = []( blob_algo::contour_t& in, Contour& out )
      {
        for ( int i = 0; i < in.count; i++ )
          out.emplace_back(  in.points[i * sizeof( int16_t )], in.points[( i * sizeof( int16_t ) ) + 1]  );
      };

      for ( int i = 0; i < blob_count; i++ )
      {
        MapComponent comp;
        comp.label = blob_out[i].label;
        fnConvertContour( blob_out[i].external, comp.contour );
        for ( int j = 0; j < blob_out[i].internal_count; j++ )
        {
          Contour cnt;
          fnConvertContour( blob_out[i].internal[j], cnt );
          comp.holes.push_back( cnt );
        }
        components_out.push_back( comp );
      }

      free( image_input );
      free( lbl_out );

      blob_algo::destroy_blobs( blob_out, blob_count );
    }

  }

}