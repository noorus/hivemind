#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "player.h"
#include "baselocation.h"
#include "messaging.h"

namespace hivemind {

  struct MyStructure {
    GameTime created_;
    GameTime completed_;
    GameTime destroyed_;
    MyStructure();
  };

  using MyStructureMap = std::map<UnitRef, MyStructure>;

  class Myself: public Subsystem, Listener {
  private:
    MyStructureMap structures_;
  public:
    Myself( Bot* bot );
    inline const MyStructure& structure( UnitRef unit ) { return structures_[unit]; }
    void gameBegin() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
    void onMessage( const Message& msg ) final;
    void gameEnd() final;
  };

}