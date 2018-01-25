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

#ifdef HIVE_SUPPORT_GUI

    namespace gdip {
      using namespace Gdiplus;
    }

    const wchar_t cRichEditControl[] = L"RichEdit50W";
    // const wchar_t cRichEditControl[] = L"RichEdit60W";

    //! Get generic window text as widestring
    inline wstring getWindowText( HWND wnd ) throw( )
    {
      wstring str;
      int length = GetWindowTextLengthW( wnd );
      if ( !length )
        return str;
      str.resize( length + 1, '\0' );
      GetWindowTextW( wnd, &str[0], length + 1 );
      str.resize( length );
      return str;
    }

    // Window

    Window::Window( HINSTANCE instance, WNDPROC wndproc, void* userdata ):
      instance_( instance ), wndProc_( wndproc ), userData_( userdata ), handle_( nullptr ), class_( 0 )
    {
    }

    Window::~Window()
    {
      if ( handle_ )
        DestroyWindow( handle_ );
      if ( class_ )
        UnregisterClassW( (LPCWSTR)class_, instance_ );
    }

    void Window::create( const string& classname, const string& title, int x, int y, int w, int h )
    {
      wstring wideClass = utf8ToWide( classname );
      wstring wideTitle = utf8ToWide( title );

      WNDCLASSEXW cls = { 0 };
      cls.cbSize = sizeof( WNDCLASSEXW );
      cls.lpfnWndProc = wndProc_;
      cls.hInstance = instance_;
      cls.hIcon = nullptr;
      cls.lpszClassName = wideClass.c_str();
      class_ = RegisterClassExW( &cls );
      if ( !class_ )
        HIVE_EXCEPT( "Window class registration failed" );

      bool sizable = true;
      bool maximizable = true;
      DWORD style = WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW | WS_POPUP;
      sizable ? style |= WS_SIZEBOX : style &= ~WS_SIZEBOX;
      maximizable ? style |= WS_MAXIMIZEBOX : style &= ~WS_MAXIMIZEBOX;

      handle_ = CreateWindowExW( 0, (LPCWSTR)class_, wideTitle.c_str(), style, x, y, w, h, nullptr, nullptr, instance_, userData_ );
      if ( !handle_ )
        HIVE_EXCEPT( "Window creation failed" );

      ShowWindow( handle_, SW_SHOWNORMAL );
      UpdateWindow( handle_ );
    }

    void Window::messageLoop( Event& stopEvent )
    {
      HANDLE handles[] = { stopEvent.get() };
      bool quitting = false;
      while ( !quitting )
      {
        auto wait = MsgWaitForMultipleObjects( 1, handles, FALSE, INFINITE, QS_ALLINPUT );
        if ( wait == WAIT_OBJECT_0 )
        {
          DestroyWindow( handle_ );
        }
        else if ( wait == WAIT_OBJECT_0 + 1 )
        {
          MSG msg;
          while ( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
          {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
            if ( msg.message == WM_QUIT )
              quitting = true;
          }
        }
        else
          HIVE_EXCEPT( "Wait for multiple objects failed" );
      }
    }

    // ConsoleWindow

#   define WM_HIVE_CONSOLEFLUSHBUFFER (WM_USER + 1)

    const COLORREF c_consoleBackground = RGB( 255, 255, 255 );
    const COLORREF c_consoleForeground = RGB( 10, 13, 20 );

    const string c_consoleFont = "Lucida Console";
    const long c_consoleFontSize = 160; // in TWIPs. if you can figure out what the fuck that means in human terms, congratulations.

    const string c_consoleClassname = "hiveConsole"; // does not matter

    const int headerHeight = 6;
    const int ctrlMargin = 2;
    const int cmdlineHeight = 20;
    const long minWidth = 320;
    const long minHeight = 240;

    void ConsoleWindow::History::reset()
    {
      stack.clear();
      browsing = false;
      position = 0;
    }

    ConsoleWindow::ConsoleWindow( Console* console, const string& title, int x, int y, int w, int h ):
      Window( g_instance, wndProc, this ),
      cmdline_( nullptr ), log_( nullptr ),
      console_( console )
    {
      console_->addListener( this );
      create( c_consoleClassname, title, x, y, w, h );
    }

    ConsoleWindow::~ConsoleWindow()
    {
      console_->removeListener( this );
    }

    void ConsoleWindow::onConsolePrint( Console* console, const string& str )
    {
      lock_.lock();
      linesBuffer_.push_back( str );
      lock_.unlock();
      PostMessageW( handle_, WM_HIVE_CONSOLEFLUSHBUFFER, 0, 0 );
    }

    void ConsoleWindow::flushBuffer()
    {
      lock_.lockShared();
      for ( auto& line : linesBuffer_ )
        print( line );
      lock_.unlockShared();
      lock_.lock();
      linesBuffer_.clear();
      lock_.unlock();
    }

    void ConsoleWindow::initTextControl( HWND ctrl, bool lineinput )
    {
      SendMessageW( ctrl, EM_LIMITTEXT, lineinput ? 100 : -1, 0 );

      if ( !lineinput )
      {
        SendMessageW( ctrl, EM_AUTOURLDETECT, TRUE, 0 );
        SendMessageW( ctrl, EM_SETEVENTMASK, NULL, ENM_SELCHANGE | ENM_LINK );
        SendMessageW( ctrl, EM_SETOPTIONS, ECOOP_OR,
          ECO_AUTOVSCROLL | ECO_NOHIDESEL | ECO_SAVESEL | ECO_SELECTIONBAR );
      }

      CHARFORMAT2W format;
      memset( &format, 0, sizeof( format ) );
      format.cbSize = sizeof( CHARFORMAT2W );
      format.dwMask = CFM_SIZE | CFM_OFFSET | CFM_EFFECTS | CFM_COLOR | CFM_BACKCOLOR | CFM_CHARSET | CFM_UNDERLINETYPE | CFM_FACE;
      format.yHeight = c_consoleFontSize;
      format.crTextColor = c_consoleForeground;
      format.crBackColor = c_consoleBackground;
      format.bCharSet = DEFAULT_CHARSET;
      format.bUnderlineType = CFU_UNDERLINENONE;

      wcscpy_s( format.szFaceName, LF_FACESIZE,
        utf8ToWide( c_consoleFont ).c_str() );

      SendMessageW( ctrl, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format );
      SendMessageW( ctrl, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN | EC_USEFONTINFO, NULL );

      CHARRANGE range = { -1, -1 };
      SendMessageW( ctrl, EM_EXSETSEL, 0, (LPARAM)&range );

      SETTEXTEX textex = { ST_DEFAULT, 1200 };
      SendMessageW( ctrl, EM_SETTEXTEX, (WPARAM)&textex, NULL );

      if ( lineinput )
        SendMessageW( ctrl, EM_SETEVENTMASK, NULL, ENM_CHANGE );
    }

    void ConsoleWindow::print( COLORREF color, const wstring& line )
    {
      CHARRANGE range = { -1, -1 };
      SendMessage( log_, EM_EXSETSEL, 0, (LPARAM)&range );
      CHARFORMAT2W format;
      format.cbSize = sizeof( CHARFORMAT2W );
      format.dwEffects = NULL;
      format.dwMask = CFM_COLOR | CFM_EFFECTS;
      format.crTextColor = color;
      SendMessage( log_, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format );
      SETTEXTEX textex = { ST_SELECTION | ST_UNICODE, 1200 };
      if ( line.length() < 3 || 0x000a000d != *(uint32_t*)( line.c_str() + ( line.length() - 2 ) ) )
        SendMessage( log_, EM_SETTEXTEX, (WPARAM)&textex, (LPARAM)line.c_str() );
      else
      {
        wstring feedFixedLine = L"\r\n" + line.substr( 0, line.length() - 2 );
        SendMessage( log_, EM_SETTEXTEX, (WPARAM)&textex, (LPARAM)feedFixedLine.c_str() );
      }
    }

    void ConsoleWindow::print( const wstring& line )
    {
      print( c_consoleForeground, line );
    }

    void ConsoleWindow::print( const string& line )
    {
      print( utf8ToWide( line ) );
    }

    void ConsoleWindow::clearCmdline()
    {
      SETTEXTEX textex = { ST_DEFAULT, 1200 };
      SendMessageW( cmdline_, EM_SETEVENTMASK, NULL, ENM_NONE );
      SendMessageW( cmdline_, EM_SETTEXTEX, (WPARAM)&textex, (LPARAM)NULL );
      SendMessageW( cmdline_, EM_SETEVENTMASK, NULL, ENM_CHANGE );
    }

    void ConsoleWindow::setCmdline( const string& line )
    {
      auto wideLine = utf8ToWide( line );

      SETTEXTEX textex = { ST_DEFAULT, 1200 };
      SendMessageW( cmdline_, EM_SETEVENTMASK, NULL, ENM_NONE );
      SendMessageW( cmdline_, EM_SETTEXTEX, (WPARAM)&textex, (LPARAM)wideLine.c_str() );
      SendMessageW( cmdline_, EM_SETEVENTMASK, NULL, ENM_CHANGE );
    }

    inline gdip::RectF makeRectf( LONG left, LONG top, LONG right, LONG bottom )
    {
      return gdip::RectF( (float)left, (float)top, (float)right - left, (float)bottom - top );
    }

    void ConsoleWindow::paint( HWND wnd, HDC hdc, RECT& area )
    {
      auto width = ( area.right - area.left );
      auto height = ( area.bottom - area.top );

      HDC memdc = CreateCompatibleDC( hdc );
      BITMAPINFOHEADER bmpInfoHdr = { sizeof( BITMAPINFOHEADER ), width, -height, 1, 32 };
      HBITMAP hbitmap = CreateDIBSection( memdc, (BITMAPINFO*)( &bmpInfoHdr ), DIB_RGB_COLORS, 0, 0, 0 );
      HGDIOBJ oldbitmap = SelectObject( memdc, hbitmap );

      // ---

      gdip::Graphics gfx( memdc );

      auto header = makeRectf( area.left, area.top, area.right, headerHeight );
      auto client = makeRectf( area.left, area.top + headerHeight, area.right, area.bottom );

      static const gdip::SolidBrush brush_consoleHeader( gdip::Color( 255, 255, 46, 115 ) );
      static const gdip::SolidBrush brush_consoleBg( gdip::Color( 255, 240, 240, 240 ) );
      static const gdip::SolidBrush brush_transWhite( gdip::Color( 175, 255, 255, 255 ) );
      static const gdip::SolidBrush brush_transBlack( gdip::Color( 100, 0, 0, 0 ) );

      gfx.FillRectangle( &brush_consoleHeader, header );

      gfx.FillRectangle( &brush_transWhite, 0, 0, width, 1 );
      gfx.FillRectangle( &brush_transBlack, 0, headerHeight - 1, width, 1 );

      gfx.FillRectangle( &brush_consoleBg, client );

      // ---

      BitBlt( hdc, 0, 0, area.right, area.bottom, memdc, 0, 0, SRCCOPY );
      SelectObject( memdc, oldbitmap );
      DeleteObject( hbitmap );
      DeleteDC( memdc );
    }

    inline RECT fitLogControl( int w, int h )
    {
      RECT ret;
      ret.left = ctrlMargin;
      ret.right = w - ret.left - ctrlMargin;
      ret.top = headerHeight + ctrlMargin;
      ret.bottom = h - ret.top - cmdlineHeight - ( ctrlMargin * 2 );
      return ret;
    }

    inline RECT fitCmdlineControl( int w, int h )
    {
      RECT ret;
      ret.left = ctrlMargin;
      ret.right = w - ret.left - ctrlMargin;
      ret.top = h - cmdlineHeight - ctrlMargin;
      ret.bottom = cmdlineHeight;
      return ret;
    }

    void ConsoleWindow::forwardExecute( const wstring& command )
    {
      console_->queueCommand( wideToUtf8( command ) );
    }

    LRESULT ConsoleWindow::cmdlineProc( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam )
    {
      auto window = (ConsoleWindow*)GetWindowLongPtrW( wnd, GWLP_USERDATA );

      CHARRANGE range = { 0, -1 };

      if ( msg == WM_CHAR && wparam == TAB )
        return 0;
      else if ( msg == WM_KEYDOWN )
      {
        if ( wparam == VK_TAB )
        {
          //
        }
        else if ( wparam == VK_UP && !window->history_.stack.empty() )
        {
          if ( window->history_.browsing )
          {
            if ( window->history_.position <= 0 )
            {
              window->history_.position = 0;
              SendMessageW( window->cmdline_, EM_EXSETSEL, 0, (LPARAM)&range );
              return 0;
            }
            window->history_.position--;
            auto backline = window->history_.stack[window->history_.position];
            window->setCmdline( backline.c_str() );
            SendMessageW( window->cmdline_, EM_EXSETSEL, 0, (LPARAM)&range );
          }
          else
          {
            window->history_.browsing = true;
            window->history_.position = window->history_.stack.size() - 1;
            auto backline = window->history_.stack.back();
            auto strtemp = wideToUtf8( getWindowText( window->cmdline_ ) );
            window->history_.stack.push_back( strtemp );
            window->setCmdline( backline );
            SendMessageW( window->cmdline_, EM_EXSETSEL, 0, (LPARAM)&range );
          }
          return 0;
        }
        else if ( wparam == VK_DOWN && ( !window->history_.stack.empty() || window->history_.browsing ) )
        {
          window->history_.position++;
          if ( window->history_.position >= window->history_.stack.size() )
          {
            window->history_.position = window->history_.stack.size() - 1;
            SendMessageW( window->cmdline_, EM_EXSETSEL, 0, (LPARAM)&range );
            window->history_.browsing = false;
            return 0;
          }
          auto backline = window->history_.stack[window->history_.position];
          window->setCmdline( backline );
          SendMessageW( window->cmdline_, EM_EXSETSEL, 0, (LPARAM)&range );
          if ( window->history_.position >= window->history_.stack.size() - 1 )
          {
            window->history_.stack.pop_back();
            window->history_.position = window->history_.stack.size() - 1;
            window->history_.browsing = false;
          }
          return 0;
        }
        else if ( wparam == VK_RETURN )
        {
          if ( window->history_.browsing )
          {
            window->history_.stack.pop_back();
            window->history_.position = window->history_.stack.size() - 1;
            window->history_.browsing = false;
          }
          // window->mAutocomplete.reset();
          auto line = getWindowText( window->cmdline_ );
          if ( !line.empty() )
          {
            window->forwardExecute( line );
            window->history_.stack.push_back( wideToUtf8( line ) );
            window->history_.position = window->history_.stack.size() - 1;
            window->clearCmdline();
            range.cpMin = -1;
            SendMessageW( window->cmdline_, EM_EXSETSEL, 0, (LPARAM)&range );
          }
          return 0;
        }
      }

      return CallWindowProcW( window->baseCmdlineProc_, wnd, msg, wparam, lparam );
    }

    LRESULT ConsoleWindow::wndProc( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam )
    {
      if ( msg == WM_CREATE )
      {
        auto self = (ConsoleWindow*)( (LPCREATESTRUCTW)lparam )->lpCreateParams;
        SetWindowLongPtrW( wnd, GWLP_USERDATA, (LONG_PTR)self );
        RECT rect;
        GetClientRect( wnd, &rect );

        RECT fit = fitLogControl( rect.right, rect.bottom );
        self->log_ = CreateWindowExW( WS_EX_LEFT | WS_EX_STATICEDGE, cRichEditControl, L"",
          WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_READONLY,
          fit.left, fit.top, fit.right, fit.bottom, wnd, nullptr,
          self->instance_, (void*)self );

        fit = fitCmdlineControl( rect.right, rect.bottom );
        self->cmdline_ = CreateWindowExW( WS_EX_LEFT | WS_EX_STATICEDGE,
          cRichEditControl, L"", WS_VISIBLE | WS_CHILD | ES_LEFT,
          fit.left, fit.top, fit.right, fit.bottom, wnd, nullptr,
          self->instance_, (void*)self );

        if ( !self->log_ || !self->cmdline_ )
          HIVE_EXCEPT( "Window control creation failed" );

        SetWindowLongPtrW( self->cmdline_, GWLP_USERDATA, (LONG_PTR)self );
        self->baseCmdlineProc_ = (WNDPROC)SetWindowLongPtrW(
          self->cmdline_, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)cmdlineProc );

        self->initTextControl( self->log_, false );
        self->initTextControl( self->cmdline_, true );

        auto dpi = GetDpiForSystem();
        self->dpiScaling_ = static_cast<float>( dpi ) / 96.0f;

        SetWindowPos( wnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE );

        return 0;
      }

      auto self = (ConsoleWindow*)GetWindowLongPtrW( wnd, GWLP_USERDATA );

      if ( msg == WM_ERASEBKGND )
      {
        /*RECT clientRect, topRect;
        GetClientRect( wnd, &clientRect );
        topRect = clientRect;
        topRect.bottom = headerHeight;
        clientRect.top += headerHeight;

        auto hdc = (HDC)wParam;
        FillRect( hdc, &topRect, (HBRUSH)GetStockObject( BLACK_BRUSH ) );
        FillRect( hdc, &clientRect, (HBRUSH)GetStockObject( WHITE_BRUSH ) );*/

        return 1;
      }
      else if ( msg == WM_ACTIVATE )
      {
        InvalidateRect( wnd, nullptr, true );
        return 0;
      }
      else if ( msg == WM_PAINT )
      {
        PAINTSTRUCT ps;
        auto dc = BeginPaint( wnd, &ps );

        RECT clientRect;
        GetClientRect( wnd, &clientRect );

        self->paint( wnd, dc, clientRect );

        EndPaint( wnd, &ps );

        return 0;
      }
      else if ( msg == WM_GETMINMAXINFO )
      {
        auto minmax = (LPMINMAXINFO)lparam;
        minmax->ptMinTrackSize.x = minWidth;
        minmax->ptMinTrackSize.y = minHeight;
      }
      else if ( msg == WM_EXITSIZEMOVE )
      {
        InvalidateRect( wnd, nullptr, FALSE );
        return 0;
      }
      else if ( msg == WM_SIZE )
      {
        RECT fit = fitLogControl( LOWORD( lparam ), HIWORD( lparam ) );
        MoveWindow( self->log_, fit.left, fit.top, fit.right, fit.bottom, TRUE );
        fit = fitCmdlineControl( LOWORD( lparam ), HIWORD( lparam ) );
        MoveWindow( self->cmdline_, fit.left, fit.top, fit.right, fit.bottom, TRUE );
        return 0;
      }
      else if ( msg == WM_SETFOCUS )
      {
        SetFocus( self->cmdline_ );
        return 0;
      }
      else if ( msg == WM_DESTROY )
      {
        PostQuitMessage( EXIT_SUCCESS );
        return 0;
      }
      else if ( msg == WM_HIVE_CONSOLEFLUSHBUFFER )
      {
        self->flushBuffer();
        return 0;
      }

      return DefWindowProcW( wnd, msg, wparam, lparam );
    }

#endif

  }

}

#endif // HIVE_PLATFORM_WINDOWS