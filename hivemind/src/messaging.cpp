#include "stdafx.h"
#include "messaging.h"

namespace hivemind {

  Messaging::Messaging( Bot* bot ): Subsystem( bot )
  {
  }

  void Messaging::gameBegin()
  {
    gameEnd();
  }

  void Messaging::listen( size_t groups, Listener* callback )
  {
    remove( callback );

    ListenEntry entry( groups, callback );
    if ( groups & Listen_Global )
      globalListeners_.push_back( entry );
    if ( groups & Listen_Intel )
      intelListeners_.push_back( entry );
  }

  void Messaging::remove( Listener* callback )
  {
    for ( auto it = globalListeners_.begin(); it != globalListeners_.end(); )
      if ( (*it).callback_ == callback ) { it = globalListeners_.erase( it ); } else { it++; }
    for ( auto it = intelListeners_.begin(); it != intelListeners_.end(); )
      if ( ( *it ).callback_ == callback ) { it = intelListeners_.erase( it ); } else { it++; }
  }

  void Messaging::sendGlobal( MessageCode code, int numargs, ... )
  {
    Message msg;
    msg.code = code;

    va_list va_alist;
    va_start( va_alist, numargs );
    for ( size_t i = 0; i < numargs; i++ )
    {
      void* arg = va_arg( va_alist, void* );
      msg.arguments.push_back( arg );
    }
    va_end( va_alist );

    messages_.push_back( msg );
  }

  void Messaging::update( GameTime time )
  {
    MessageVector messagesBuffer;
    messagesBuffer.swap( messages_ );

    for ( auto& msg : messagesBuffer )
    {
      if ( msg.code > M_Min_Global && msg.code < M_Max_Global ) {
        for ( auto& listener : globalListeners_ )
          listener.callback_->onMessage( msg );
      }
      else if ( msg.code > M_Min_Intel && msg.code < M_Max_Intel ) {
        for ( auto& listener : intelListeners_ )
          listener.callback_->onMessage( msg );
      }
    }
  }

  void Messaging::gameEnd()
  {
    globalListeners_.clear();
    intelListeners_.clear();
  }

}