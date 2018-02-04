#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "player.h"
#include "baselocation.h"
#include "messaging.h"

namespace hivemind {

  class Pathing: public Subsystem, Listener {
  private:
  public:
    Pathing( Bot* bot );
    void gameBegin() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
    void onMessage( const Message& msg ) final;
    void gameEnd() final;
  };

}