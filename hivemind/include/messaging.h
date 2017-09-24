#pragma once
#include "sc2_forward.h"
#include "subsystem.h"

// ignore pointer typecasting errors for Message arguments, because we know they're not pointers at all
#pragma warning(push)
#pragma warning(disable: 4311 4302)

namespace hivemind {

  enum MessageCode {
    M_Min_Global,
    M_Global_UnitCreated, //!< args: Unit
    M_Global_UnitDestroyed, //!< args: Unit
    M_Global_UnitIdle, //!< args: Unit
    M_Global_UpgradeCompleted, //!< args: UpgradeID
    M_Global_ConstructionCompleted, //!< args: Unit
    M_Global_NydusDetected, //!< args: None
    M_Global_NuclearLaunchDetected, //!< args: None
    M_Global_UnitEnterVision, //!< args: Unit
    M_Global_AddWorker, //!< args: Tag
    M_Global_RemoveWorker, //!< args: Tag
    M_Max_Global,
    M_Min_Intel,
    M_Intel_FoundPlayer, //!< args: PlayerID, Unit
    M_Max_Intel
  };

  enum ListenGroups {
    Listen_Global = 1,
    Listen_Intel = 2
  };

  struct Message {
    MessageCode code;
    vector<void*> arguments;
    inline const Unit* unit( int index = 0 ) const { return (Unit*)( arguments[index] ); }
    inline const UpgradeID upgrade( int index = 0 ) const { return (uint32_t)( arguments[index] ); }
    inline const Tag tag( int index = 0 ) const { return (Tag)( arguments[index] ); }
    inline const PlayerID player( int index = 0 ) const { return (PlayerID)( arguments[index] ); }
  };

  using MessageVector = vector<Message>;

  class Messaging;

  class Listener {
    friend class Messaging;
  private:
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
    Listeners intelListeners_;
    MessageVector messages_;
  public:
    Messaging( Bot* bot );
    void gameBegin() final;
    void gameEnd() final;
    void listen( size_t groups, Listener* callback );
    void remove( Listener* callback );
    void sendGlobal( MessageCode code, int numargs = 0, ... );
    template<typename T> void sendGlobal( MessageCode code, T arg ) { sendGlobal( code, 1, arg ); }
    void update( GameTime time );
  };

}

#pragma warning(pop)