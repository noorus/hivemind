#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"
#include "cache.h"

namespace hivemind {

  Bot* Cache::bot_ = nullptr;

  HIVE_DECLARE_CONVAR( cache_path, "Path to a directory where the bot can cache stuff.", R"(..\cache)" );

  const uint32_t cMapCacheVersion = 1;

  string makeCacheFilePath( const Sha256& hash, const string& name )
  {
    string maphash = utils::hexString( hash );
    char filename[128];
    sprintf_s( filename, 128, "%s_%s.cache", maphash.c_str(), name.c_str() );
    string path = g_CVar_cache_path.as_s() + string( "\\" ) + filename;
    return path;
  }

  using FileReaderPtr = std::shared_ptr<platform::FileReader>;

  FileReaderPtr openCacheFile( const Sha256& hash, const string& name )
  {
    auto path = makeCacheFilePath( hash, name );

    if ( !platform::fileExists( path ) )
      return FileReaderPtr();

    return std::make_shared<platform::FileReader>( path );
  }

  using FileWriterPtr = std::shared_ptr<platform::FileWriter>;

  FileWriterPtr createCacheFile( const Sha256& hash, const string& name )
  {
    auto path = makeCacheFilePath( hash, name );

    return std::make_shared<platform::FileWriter>( path );
  }

  void Cache::setBot( Bot* bot )
  {
    bot_ = bot;
  }

  bool Cache::hasMapCache( const MapData& map, const string& name )
  {
    auto path = makeCacheFilePath( map.hash, name );
    return platform::fileExists( path );
  }

  bool Cache::mapReadIntArray2( const MapData& map, Array2<int>& data, const string& name )
  {
    auto reader = openCacheFile( map.hash, name );

    if ( !reader )
      return false;

    auto version = reader->readUint32();
    if ( version != cMapCacheVersion )
    {
      bot_->console().printf( "Cache: Error; cache file version mismatch: v%d, expected v%d", version, cMapCacheVersion );
      return false;
    }

    auto size = (uint32_t)( data.width() * data.height() * sizeof( int ) );
    if ( reader->size() != ( size + sizeof( version ) ) )
    {
      bot_->console().printf( "Cache: Error; cache file size mismatch: %d, expected %d bytes", reader->size(), size );
      return false;
    }

    reader->read( (void*)data.data(), size );
    return true;
  }

  void Cache::mapWriteIntArray2( const MapData& map, Array2<int>& data, const string& name )
  {
    auto writer = createCacheFile( map.hash, name );

    if ( !writer )
      HIVE_EXCEPT( "Cache file creation failed" );

    writer->writeUint32( cMapCacheVersion );

    auto size = (uint32_t)( data.width() * data.height() * sizeof( int ) );
    writer->writeBlob( (const void*)data.data(), size );
  }

  Polygon unserializePolygon( FileReaderPtr reader )
  {
    Polygon ret;
    size_t size = reader->readUint64();
    for ( size_t i = 0; i < size; ++i )
    {
      ret.push_back( reader->readVector2() );
    }
    size_t holeCount = reader->readUint64();
    for ( size_t j = 0; j < holeCount; j++ )
      ret.holes.push_back( unserializePolygon( reader ) );
    return ret;
  }

  bool Cache::mapReadRegionVector( const MapData& map, RegionVector& regions, const string& name )
  {
    auto reader = openCacheFile( map.hash, name );

    if ( !reader )
      return false;

    auto version = reader->readUint32();
    if ( version != cMapCacheVersion )
    {
      bot_->console().printf( "Cache: Error; Cache file version mismatch: v%d, expected v%d", version, cMapCacheVersion );
      return false;
    }

    size_t size = reader->readUint64();
    for ( size_t i = 0; i < size; ++i )
    {
      auto region = new Region();
      region->label_ = reader->readInt();
      region->polygon_ = unserializePolygon( reader );
      regions.push_back( region );
    }

    return true;
  }

  void serializePolygon( Polygon& poly, FileWriterPtr writer )
  {
    size_t length = poly.size();
    writer->writeUint64( length );
    for ( size_t i = 0; i < length; ++i )
    {
      writer->writeReal( poly[i].x );
      writer->writeReal( poly[i].y );
    }
    writer->writeUint64( poly.holes.size() );
    for ( auto& hole : poly.holes )
      serializePolygon( hole, writer );
  }

  void Cache::mapWriteRegionVector( const MapData& map, RegionVector& regions, const string& name )
  {
    auto writer = createCacheFile( map.hash, name );

    if ( !writer )
      HIVE_EXCEPT( "Cache file creation failed" );

    writer->writeUint32( cMapCacheVersion );

    writer->writeUint64( regions.size() );
    for ( auto region : regions )
    {
      writer->writeInt( region->label_ );
      serializePolygon( region->polygon_, writer );
    }
  }

}