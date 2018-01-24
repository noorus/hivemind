#pragma once
#include "sc2_forward.h"
#include "exception.h"

namespace hivemind {

  namespace platform {

    //! \class RWLock
    //! Reader-writer lock class for easy portability.
    class RWLock: boost::noncopyable {
    protected:
      SRWLOCK mLock;
    public:
      RWLock() { InitializeSRWLock( &mLock ); }
      void lock() { AcquireSRWLockExclusive( &mLock ); }
      void unlock() { ReleaseSRWLockExclusive( &mLock ); }
      void lockShared() { AcquireSRWLockShared( &mLock ); }
      void unlockShared() { ReleaseSRWLockShared( &mLock ); }
    };

    class PerformanceTimer {
    private:
      LARGE_INTEGER frequency_;
      LARGE_INTEGER time_;
    public:
      PerformanceTimer()
      {
        QueryPerformanceFrequency( &frequency_ );
      }
      void start()
      {
        QueryPerformanceCounter( &time_ );
      }
      double stop()
      {
        LARGE_INTEGER newTime;
        QueryPerformanceCounter( &newTime );
        auto delta = ( newTime.QuadPart - time_.QuadPart ) * 1000000;
        delta /= frequency_.QuadPart;
        double ms = (double)delta / 1000.0;
        return ms;
      }
    };

    class FileReader {
    protected:
      HANDLE file_;
      uint64_t size_;
    public:
      FileReader( const string& filename ): file_( INVALID_HANDLE_VALUE )
      {
        file_ = CreateFileA( filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
        if ( file_ == INVALID_HANDLE_VALUE )
          HIVE_EXCEPT( "File open failed" );

        DWORD sizeHigh = 0;
        DWORD sizeLow = GetFileSize( file_, &sizeHigh );

        size_ = ( (uint64_t)sizeHigh << 32 | sizeLow );

        SetFilePointer( file_, 0, NULL, FILE_BEGIN );
      }
      inline const uint64_t size() const { return size_; }
      ~FileReader()
      {
        if ( file_ != INVALID_HANDLE_VALUE )
          CloseHandle( file_ );
      }
      void read( void* out, uint32_t length )
      {
        DWORD read = 0;
        if ( ReadFile( file_, out, length, &read, nullptr ) != TRUE || read < length )
          HIVE_EXCEPT( "File read failed or length mismatch" );
      }
      uint64_t readUint64()
      {
        uint64_t ret;
        DWORD read = 0;
        if ( ReadFile( file_, &ret, sizeof( uint64_t ), &read, nullptr ) != TRUE || read < sizeof( uint64_t ) )
          HIVE_EXCEPT( "File read failed or length mismatch" );
        return ret;
      }
      uint32_t readUint32()
      {
        uint32_t ret;
        DWORD read = 0;
        if ( ReadFile( file_, &ret, sizeof( uint32_t ), &read, nullptr ) != TRUE || read < sizeof( uint32_t ) )
          HIVE_EXCEPT( "File read failed or length mismatch" );
        return ret;
      }
      int readInt()
      {
        int ret;
        DWORD read = 0;
        if ( ReadFile( file_, &ret, sizeof( int ), &read, nullptr ) != TRUE || read < sizeof( int ) )
          HIVE_EXCEPT( "File read failed or length mismatch" );
        return ret;
      }
      Real readReal()
      {
        Real ret;
        DWORD read = 0;
        if ( ReadFile( file_, &ret, sizeof( Real ), &read, nullptr ) != TRUE || read < sizeof( Real ) )
          HIVE_EXCEPT( "File read failed or length mismatch" );
        return ret;
      }
      Vector2 readVector2()
      {
        Real vals[2];
        DWORD read = 0;
        if ( ReadFile( file_, vals, sizeof( Real ) * 2, &read, nullptr ) != TRUE || read < ( sizeof( Real ) * 2 ) )
          HIVE_EXCEPT( "File read failed or length mismatch" );
        return Vector2( vals[0], vals[1] );
      }
    };

    class FileWriter {
    protected:
      HANDLE file_;
    public:
      FileWriter( const string& filename ): file_( INVALID_HANDLE_VALUE )
      {
        file_ = CreateFileA( filename.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
        if ( file_ == INVALID_HANDLE_VALUE )
          HIVE_EXCEPT( "File creation failed" );
      }
      ~FileWriter()
      {
        if ( file_ != INVALID_HANDLE_VALUE )
          CloseHandle( file_ );
      }
      void writeInt( int value )
      {
        DWORD written = 0;
        auto ret = WriteFile( file_, &value, sizeof( int ), &written, nullptr );
        if ( !ret || written != sizeof( int ) )
          HIVE_EXCEPT( "Failed to write data" );
      }
      void writeUint64( uint64_t value )
      {
        DWORD written = 0;
        auto ret = WriteFile( file_, &value, sizeof( uint64_t ), &written, nullptr );
        if ( !ret || written != sizeof( uint64_t ) )
          HIVE_EXCEPT( "Failed to write data" );
      }
      void writeUint32( uint32_t value )
      {
        DWORD written = 0;
        auto ret = WriteFile( file_, &value, sizeof( uint32_t ), &written, nullptr );
        if ( !ret || written != sizeof( uint32_t ) )
          HIVE_EXCEPT( "Failed to write data" );
      }
      void writeReal( Real value )
      {
        DWORD written = 0;
        auto ret = WriteFile( file_, &value, sizeof( Real ), &written, nullptr );
        if ( !ret || written != sizeof( Real ) )
          HIVE_EXCEPT( "Failed to write data" );
      }
      void writeBlob( const void* buffer, uint32_t length )
      {
        DWORD written = 0;
        auto ret = WriteFile( file_, buffer, length, &written, nullptr );
        if ( !ret || written != length )
          HIVE_EXCEPT( "Failed to write data" );
      }
    };

    inline bool fileExists( const string& path )
    {
      DWORD attributes = GetFileAttributesA( path.c_str() );
      return ( attributes != INVALID_FILE_ATTRIBUTES && !( attributes & FILE_ATTRIBUTE_DIRECTORY ) );
    }

  }

}