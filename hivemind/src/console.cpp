#include "stdafx.h"
#include "console.h"
#include "utilities.h"
#include "exception.h"
#include "bot.h"

namespace hivemind {

  const char* c_fileLogFormat = "%06d [%02d:%02d] %s\r\n";
  const int c_sprintfBufferSize = 1024;

  CVarList Console::precreated_;

  // ConBase

  ConBase::ConBase( const string& name, const string& description ):
    name_( name ), description_( description ), registered_( false )
  {
    assert( !name_.empty() && !description_.empty() );

    Console::precreated_.push_back( this );
  }

  void ConBase::onRegister()
  {
    registered_ = true;
  }

  // ConCmd

  ConCmd::ConCmd( const string& name, const string& description,
    ConCmd::Callback callback ):
    ConBase( name, description ), callback_( callback )
  {
    assert( callback_ );
  }

  void ConCmd::call( Console* console, StringVector& arguments )
  {
    callback_( console, this, arguments );
  }

  // ConVar

  ConVar::ConVar( const string& name, const string& description,
    int defaultValue, ConVar::Callback callback ):
    ConBase( name, description ), callback_( callback )
  {
    set( defaultValue );
    default_ = value_;
  }

  ConVar::ConVar( const string& name, const string& description,
    float defaultValue, ConVar::Callback callback ):
    ConBase( name, description ), callback_( callback )
  {
    set( defaultValue );
    default_ = value_;
  }

  ConVar::ConVar( const string& name, const string& description,
    const string& defaultValue, ConVar::Callback callback ):
    ConBase( name, description ), callback_( callback )
  {
    set( defaultValue );
    default_ = value_;
  }

  void ConVar::set( int value )
  {
    Value oldValue = value_;
    value_.i = value;
    value_.f = (float)value;
    char strtmp[32];
    sprintf_s( strtmp, 32, "%i", value );
    value_.str = strtmp;
    if ( !registered_ || !callback_ || callback_( this, oldValue ) )
      return;
    value_ = oldValue;
  }

  void ConVar::set( float value )
  {
    Value oldValue = value_;
    value_.i = (int)value;
    value_.f = value;
    char strtmp[32];
    sprintf_s( strtmp, 32, "%f", value );
    value_.str = strtmp;
    if ( !registered_ || !callback_ || callback_( this, oldValue ) )
      return;
    value_ = oldValue;
  }

  void ConVar::set( const string& value )
  {
    Value oldValue = value_;
    value_.i = atoi( value.c_str() );
    value_.f = (float)atof( value.c_str() );
    value_.str = value;
    if ( !registered_ || !callback_ || callback_( this, oldValue ) )
      return;
    value_ = oldValue;
  }

  void ConVar::forceSet( int value )
  {
    value_.i = value;
    value_.f = (float)value;
    char strtmp[32];
    sprintf_s( strtmp, 32, "%i", value );
    value_.str = strtmp;
  }

  void ConVar::forceSet( float value )
  {
    value_.i = (int)value;
    value_.f = value;
    char strtmp[32];
    sprintf_s( strtmp, 32, "%f", value );
    value_.str = strtmp;
  }

  void ConVar::forceSet( const string& value )
  {
    value_.i = atoi( value.c_str() );
    value_.f = (float)atof( value.c_str() );
    value_.str = value;
  }

  // TextFile

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

  Console::Console(): bot_( nullptr ), fileOut_( nullptr )
  {
    for ( ConBase* var : precreated_ )
      registerVariable( var );

    precreated_.clear();
  }

  void Console::setBot( Bot* bot )
  {
    if ( bot_ || bot_ != bot )
      HIVE_EXCEPT( "A bot is already assigned to Console" );

    bot_ = bot;
  }

  void Console::registerVariable( ConBase* var )
  {
    ScopedRWLock lock( &lock_ );

    auto it = std::find( cvars_.begin(), cvars_.end(), var );
    if ( it != cvars_.end() || var->isRegistered() )
      HIVE_EXCEPT( "CVAR has already been registered" );

    var->onRegister();
    cvars_.push_back( var );
  }

  void Console::gameBegin()
  {
    assert( bot_ );

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

    if ( fileOut_ )
      fileOut_->write( fullbuf );
    ::printf( fullbuf );
  #ifdef _DEBUG
    OutputDebugStringA( fullbuf );
  #endif
  }

  StringVector Console::tokenize( const string& str )
  {
    // this implementation is naïve, but hardly critical

    bool quoted = false;
    bool escaped = false;

    string buffer;
    StringVector v;

    for ( char chr : str )
    {
      if ( chr == BACKSLASH )
      {
        if ( escaped )
          buffer.append( 1, chr );
        escaped = !escaped;
      }
      else if ( chr == SPACE )
      {
        if ( !quoted )
        {
          if ( !buffer.empty() )
          {
            v.push_back( buffer );
            buffer.clear();
          }
        } else
          buffer.append( 1, chr );
        escaped = false;
      }
      else if ( chr == QUOTE )
      {
        if ( escaped )
        {
          buffer.append( 1, chr );
          escaped = false;
        }
        else
        {
          if ( quoted )
          {
            if ( !buffer.empty() )
            {
              v.push_back( buffer );
              buffer.clear();
            }
          }
          quoted = !quoted;
        }
      }
      else
      {
        buffer.append( 1, chr );
        escaped = false;
      }
    }

    if ( !buffer.empty() )
      v.push_back( buffer );

    return v;
  }

  void Console::execute( string commandLine, const bool echo )
  {
    boost::trim( commandLine );
    if ( commandLine.empty() )
      return;

    auto arguments = tokenize( commandLine );
    if ( arguments.empty() )
      return;

    if ( echo )
      printf( "> %s", commandLine.c_str() );

    ScopedRWLock lock( &lock_ );

    auto command = arguments[0];
    for ( auto base : cvars_ )
    {
      if ( !base->isRegistered() )
        continue;
      if ( boost::iequals( base->name(), command ) )
      {
        if ( base->isCommand() )
        {
          auto cmd = static_cast<ConCmd*>( base );
          lock.unlock();
          cmd->call( this, arguments );
        }
        else
        {
          auto var = static_cast<ConVar*>( base );
          if ( arguments.size() > 1 )
            var->set( arguments[1] );
          else
          {
            lock.unlock();
            printf( R"(%s is "%s")",
              var->name().c_str(),
              var->as_s().c_str() );
          }
        }
        return;
      }
    }

    lock.unlock();

    printf( R"(Unknown command "%s")", command.c_str() );
  }

  void Console::gameEnd()
  {
    assert( bot_ );

    writeStopBanner();
    SAFE_DELETE( fileOut_ );

    bot_ = nullptr;
  }

}