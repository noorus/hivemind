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

  //! Cache file version number, increment this to invalidate old caches if the format changes
  const uint32_t cMapCacheVersion = 7;

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

  inline Region* regionByLabel( const RegionVector& regs, int label )
  {
    for ( auto reg : regs )
      if ( reg->label_ == label )
        return reg;

    return nullptr;
  }

  inline Vector2 unserializeVector2( FileReaderPtr reader )
  {
    Vector2 ret;
    ret.x = reader->readReal();
    ret.y = reader->readReal();
    return ret;
  }

  inline void serializeVector2( const Vector2& vec, FileWriterPtr writer )
  {
    writer->writeReal( vec.x );
    writer->writeReal( vec.y );
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

  bool Cache::mapReadUint64Array2( const MapData & map, Array2<uint64_t>& data, const string & name )
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

    auto size = (uint32_t)( data.width() * data.height() * sizeof( uint64_t ) );
    if ( reader->size() != ( size + sizeof( version ) ) )
    {
      bot_->console().printf( "Cache: Error; cache file size mismatch: %d, expected %d bytes", reader->size(), size );
      return false;
    }

    reader->read( (void*)data.data(), size );
    return true;
  }

  void Cache::mapWriteUint64Array2( const MapData & map, Array2<uint64_t>& data, const string & name )
  {
    auto writer = createCacheFile( map.hash, name );

    if ( !writer )
      HIVE_EXCEPT( "Cache file creation failed" );

    writer->writeUint32( cMapCacheVersion );

    auto size = (uint32_t)( data.width() * data.height() * sizeof( uint64_t ) );
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
      region->tileCount_ = reader->readInt();
      region->height_ = reader->readReal();
      region->heightLevel_ = reader->readInt();
      region->dubious_ = reader->readBool();
      region->opennessDistance_ = reader->readReal();
      region->opennessPoint_ = unserializeVector2( reader );
      regions.push_back( region );
    }

    for ( size_t i = 0; i < size; ++i )
    {
      size_t reachable = reader->readUint64();
      for ( size_t j = 0; j < reachable; j++ )
      {
        size_t index = reader->readUint64();
        auto reachReg = regions[index];
        regions[i]->reachableRegions_.insert( reachReg );
      }
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
      writer->writeInt( region->tileCount_ );
      writer->writeReal( region->height_ );
      writer->writeInt( region->heightLevel_ );
      writer->writeBool( region->dubious_ );
      writer->writeReal( region->opennessDistance_ );
      serializeVector2( region->opennessPoint_, writer );
    }

    for ( auto region : regions )
    {
      writer->writeUint64( region->reachableRegions_.size() );
      for ( auto reach : region->reachableRegions_ )
      {
        writer->writeUint64( reach->label_ );
      }
    }
  }

  bool Cache::mapReadChokeVector( const MapData& map, ChokeVector& chokes, RegionVector& regions, const string& name )
  {
    auto reader = openCacheFile( map.hash, name );

    if ( !reader )
      return false;

    assert( chokes.empty() );
    assert( !regions.empty() );

    auto version = reader->readUint32();
    if ( version != cMapCacheVersion )
    {
      bot_->console().printf( "Cache: Error; Cache file version mismatch: v%d, expected v%d", version, cMapCacheVersion );
      return false;
    }

    size_t size = reader->readUint64();
    for ( size_t i = 0; i < size; ++i )
    {
      Chokepoint choke;
      choke.id_ = reader->readInt();
      choke.region1 = regionByLabel( regions, reader->readInt() );
      choke.region2 = regionByLabel( regions, reader->readInt() );

      if ( !choke.region1 || !choke.region2 )
        return false;

      choke.side1 = unserializeVector2( reader );
      choke.side2 = unserializeVector2( reader );
      chokes.push_back( choke );
    }

    return true;
  }

  void Cache::mapWriteChokeVector( const MapData& map, ChokeVector& chokes, const string& name )
  {
    auto writer = createCacheFile( map.hash, name );

    if ( !writer )
      HIVE_EXCEPT( "Cache file creation failed" );

    writer->writeUint32( cMapCacheVersion );

    writer->writeUint64( chokes.size() );
    for ( const auto& choke : chokes )
    {
      writer->writeInt( choke.id_ );
      writer->writeInt( choke.region1->label_ );
      writer->writeInt( choke.region2->label_ );
      serializeVector2( choke.side1, writer );
      serializeVector2( choke.side2, writer );
    }
  }

  bool Cache::mapReadRegionGraph( const MapData& map, OptimalRegionGraph& graph, const string& name )
  {
    // TODO
    return true;
  }

  void Cache::mapWriteRegionGraph( const MapData& map, OptimalRegionGraph& graph, const string& name )
  {
    // TODO
  }

}