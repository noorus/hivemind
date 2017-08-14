#include "stdafx.h"
#include "console.h"
#include "utilities.h"
#include "exception.h"
#include "bot.h"

namespace hivemind {

  const char* c_fileLogFormat = "%06d [%02d:%02d] %s\r\n";
  const int c_sprintfBufferSize = 1024;

  struct DumbDate {
    uint16_t year;    //!< Year
    uint16_t month;   //!< Month
    uint16_t day;     //!< Day
    uint16_t hour;    //!< Hour
    uint16_t minute;  //!< Minute
    uint16_t second;  //!< Second
  };

  void getDateTime( DumbDate& out )
  {
    SYSTEMTIME time;
    GetLocalTime( &time );
    out.year = time.wYear;
    out.month = time.wMonth;
    out.day = time.wDay;
    out.hour = time.wHour;
    out.minute = time.wMinute;
    out.second = time.wSecond;
  }

  TextFile::TextFile( const string& filename ): file_( INVALID_HANDLE_VALUE )
  {
    file_ = CreateFileA( filename.c_str(), GENERIC_WRITE,
      FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, 0 );

    if ( file_ == INVALID_HANDLE_VALUE )
      HIVE_EXCEPT( "Cannot create text file" );

    SetFilePointer( file_, 0, nullptr, FILE_END );
  }

  void TextFile::write( const string& str )
  {
    DWORD written;
    if ( !WriteFile( file_, str.c_str(), (DWORD)str.size(), &written, nullptr ) )
      HIVE_EXCEPT( "Cannot write to file" );
  }

  TextFile::~TextFile()
  {
    SAFE_CLOSE_HANDLE( file_ );
  }

  Console::Console( Bot* bot ): Subsystem( bot ), fileOut_( nullptr )
  {
  }

  void Console::gameBegin()
  {
    DumbDate now;
    getDateTime( now );

    SAFE_DELETE( fileOut_ );

    char filename[64];
    sprintf_s( filename, 64, "%04d%02d%02d_hivemind_game-%02d%02d%02d.log",
      now.year, now.month, now.day, now.hour, now.minute, now.second );

    fileOut_ = new TextFile( filename );

    writeStartBanner();
  }

  void Console::writeStartBanner()
  {
    DumbDate now;
    getDateTime( now );

    printf( "HIVEMIND StarCraft II AI [Debug Log]" );
    printf( "Game starting on %04d-%02d-%02d %02d:%02d:%02d",
      now.year, now.month, now.day, now.hour, now.minute, now.second );
  }

  void Console::writeStopBanner()
  {
    DumbDate now;
    getDateTime( now );

    printf( "Game ending on %04d-%02d-%02d %02d:%02d:%02d",
      now.year, now.month, now.day, now.hour, now.minute, now.second );
  }

  void Console::printf( const char* str, ... )
  {
    va_list va_alist;
    char buffer[c_sprintfBufferSize];
    va_start( va_alist, str );
    _vsnprintf_s( buffer, c_sprintfBufferSize, str, va_alist );
    va_end( va_alist );

    auto ticks = bot_->time();
    auto realTime = utils::ticksToTime( ticks );
    unsigned int minutes = ( realTime / 60 );
    unsigned int seconds = ( realTime % 60 );

    char fullbuf[c_sprintfBufferSize + 128];
    sprintf_s( fullbuf, c_sprintfBufferSize + 128, c_fileLogFormat, ticks, minutes, seconds, buffer );

    fileOut_->write( fullbuf );
    ::printf( fullbuf );
  }

  void Console::gameEnd()
  {
    writeStopBanner();
    SAFE_DELETE( fileOut_ );
  }

}