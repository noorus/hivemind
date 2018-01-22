#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"
#include "..\include\cache.h"

namespace hivemind {

  Bot* Cache::bot_ = nullptr;

  HIVE_DECLARE_CONVAR( cache_path, "Path to a directory where the bot can cache stuff.", R"(..\cache)" );

  string makeCacheFilePath( const Sha256& hash, const string& name )
  {
    string maphash = utils::hexString( hash );
    char filename[128];
    sprintf_s( filename, 128, "%s_%s.cache", maphash.c_str(), name.c_str() );
    string path = g_CVar_cache_path.as_s() + string( "\\" ) + filename;
    return path;
  }

  using FileReaderPtr = std::unique_ptr<platform::FileReader>;

  FileReaderPtr openCacheFile( const Sha256& hash, const string& name )
  {
    auto path = makeCacheFilePath( hash, name );

    if ( !platform::fileExists( path ) )
      return FileReaderPtr();

    return std::make_unique<platform::FileReader>( path );
  }

  using FileWriterPtr = std::unique_ptr<platform::FileWriter>;

  FileWriterPtr createCacheFile( const Sha256& hash, const string& name )
  {
    auto path = makeCacheFilePath( hash, name );

    return std::make_unique<platform::FileWriter>( path );
  }

  void Cache::setBot( Bot* bot )
  {
    bot_ = bot;
  }

  bool Cache::mapReadIntArray2( const MapData& map, Array2<int>& data, const string& name )
  {
    auto reader = openCacheFile( map.hash, name );

    if ( !reader )
      return false;

    auto size = (uint32_t)( data.width() * data.height() * sizeof( int ) );
    if ( reader->size() != size )
    {
      bot_->console().printf( "Cache: Error; Cache file size mismatch: %d, expected %d bytes", reader->size(), size );
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

    auto size = (uint32_t)( data.width() * data.height() * sizeof( int ) );
    writer->writeBlob( (const void*)data.data(), size );
  }

}