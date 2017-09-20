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
    ListenEntry entry( groups, callback );
    if ( groups & Listen_Global )
      globalListeners_.push_back( entry );
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
    }
  }

  void Messaging::gameEnd()
  {
    globalListeners_.clear();
  }

}