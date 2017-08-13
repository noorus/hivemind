#pragma once
#include "sc2_forward.h"
#include "brain.h"

// ignore pointer typecasting errors for Message arguments, because we know they're not pointers at all
#pragma warning(push)
#pragma warning(disable: 4311 4302)

namespace hivemind {

  enum MessageCode {
    M_Min_Global,
    M_Global_UnitCreated,
    M_Global_UnitDestroyed,
    M_Global_UnitIdle,
    M_Global_UpgradeCompleted,
    M_Global_ConstructionCompleted,
    M_Global_NydusDetected,
    M_Global_NuclearLaunchDetected,
    M_Global_UnitEnterVision,
    M_Max_Global
  };

  enum ListenGroups {
    Listen_Global = 1
  };

  struct Message {
    MessageCode code;
    vector<void*> arguments;
    inline const Unit* unit( int index = 0 ) const { return (Unit*)( arguments[index] ); }
    inline const UpgradeID upgrade( int index = 0 ) const { return (uint32_t)( arguments[index] ); }
  };

  using MessageVector = vector<Message>;

  class Listener {
  public:
    virtual void onMessage( const Message& msg ) = 0;
  };

  // NOTE process HAS to be called same frame as the sends, or pointers in args might get invalidated
  class Messaging: public Subsystem {
  public:
    struct ListenEntry {
      size_t groups_;
      Listener* callback_;
      ListenEntry( size_t groups, Listener* callback ): groups_( groups ), callback_( callback ) {}
    };
    using Listeners = vector<ListenEntry>;
  private:
    Listeners globalListeners_;
    MessageVector messages_;
  public:
    Messaging( Bot* bot );
    void gameBegin() final;
    void gameEnd() final;
    void listen( size_t groups, Listener* callback );
    void sendGlobal( MessageCode code, int numargs = 0, ... );
    template<typename T> void sendGlobal( MessageCode code, T arg ) { sendGlobal( code, 1, arg ); }
    void update( GameTime time );
  };

}

#pragma warning(pop)