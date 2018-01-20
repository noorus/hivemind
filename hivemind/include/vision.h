#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "player.h"
#include "baselocation.h"
#include "messaging.h"

namespace hivemind {

  class Vision: public Subsystem, Listener {
  public:
    struct StartLocations {
      set<size_t> unexplored_;
      set<size_t> explored_;
    };
  private:
    StartLocations startLocations_;
  public:
    Vision( Bot* bot );
    void gameBegin() final;
    void update( const GameTime time, const GameTime delta );
    void onMessage( const Message& msg ) final;
    inline StartLocations& startLocations() { return startLocations_; }
    void gameEnd() final;
  };

}