#include "stdafx.h"
#include "console.h"
#include "consolewindow_windows.h"

#ifdef HIVE_PLATFORM_WINDOWS

#include "platform_windows.h"
#include "exception.h"

#include <windows.h>
#include <unknwn.h>
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

namespace hivemind {

  namespace platform {

    HINSTANCE g_instance = nullptr;

#ifdef HIVE_SUPPORT_GUI

    const wchar_t cRichEditDLL[] = L"msftedit.dll";
    // const wchar_t cRichEditDLL[] = L"riched20.dll";

    static HMODULE g_richEditLibrary = nullptr;
    static ULONG_PTR g_gdiplusToken = 0;

    namespace gdip {
      using namespace Gdiplus;
    }

    void initializeGUI()
    {
      INITCOMMONCONTROLSEX ctrls;
      ctrls.dwSize = sizeof( INITCOMMONCONTROLSEX );
      ctrls.dwICC = ICC_STANDARD_CLASSES | ICC_NATIVEFNTCTL_CLASS;

      if ( !InitCommonControlsEx( &ctrls ) )
        HIVE_EXCEPT( "Common controls init failed, missing manifest?" );

      g_richEditLibrary = LoadLibraryW( cRichEditDLL );
      if ( !g_richEditLibrary )
        HIVE_EXCEPT( "RichEdit library load failed" );

      gdip::GdiplusStartupInput gdiplusStartupInput;
      gdiplusStartupInput.SuppressExternalCodecs = TRUE;
      gdip::GdiplusStartup( &g_gdiplusToken, &gdiplusStartupInput, nullptr );
    }

    void shutdownGUI()
    {
      if ( g_gdiplusToken )
        gdip::GdiplusShutdown( g_gdiplusToken );

      if ( g_richEditLibrary )
        FreeLibrary( g_richEditLibrary );
    }

  #else

    inline void initializeGUI() {}
    inline void shutdownGUI() {}

  #endif // HIVE_SUPPORT_GUI

    void initialize()
    {
      initializeGUI();
    }

    void prepareProcess()
    {
      SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

      // SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

      // DWORD index = 0;
      // AvSetMmThreadCharacteristicsW( L"Games", &index );
    }

    void shutdown()
    {
      shutdownGUI();
    }

    // Thread

    Thread::Thread( const string& name, Callback callback, void* argument ):
      thread_( nullptr ), id_( 0 ), name_( name ), callback_( callback ), argument_( argument )
    {
    }

    DWORD Thread::threadProc( void* argument )
    {
      auto self = (Thread*)argument;
      auto ret = self->callback_( self->run_, self->stop_, self->argument_ );
      return ( ret ? EXIT_SUCCESS : EXIT_FAILURE );
    }

    bool Thread::start()
    {
      thread_ = CreateThread( nullptr, 0, threadProc, this, CREATE_SUSPENDED, &id_ );
      if ( !thread_ )
        return false;

      setDebuggerThreadName( id_, name_ );

      if ( ResumeThread( thread_ ) == -1 )
        HIVE_EXCEPT( "Thread resume failed" );

      HANDLE events[2] = {
        run_.get(), thread_
      };

      auto wait = WaitForMultipleObjects( 2, events, FALSE, INFINITE );
      if ( wait == WAIT_OBJECT_0 )
      {
        return true;
      }
      else if ( wait == WAIT_OBJECT_0 + 1 )
      {
        return false;
      }
      else
        HIVE_EXCEPT( "Thread wait failed" );

      return false;
    }

    void Thread::stop()
    {
      if ( thread_ )
      {
        stop_.set();
        WaitForSingleObject( thread_, INFINITE );
      }
      stop_.reset();
      run_.reset();
      thread_ = nullptr;
      id_ = 0;
    }

    bool Thread::waitFor( uint32_t milliseconds ) const
    {
      auto retval = WaitForSingleObject( thread_, milliseconds );
      return ( retval == WAIT_TIMEOUT );
    }

    Thread::~Thread()
    {
      stop();
    }

  }

}

#endif // HIVE_PLATFORM_WINDOWS