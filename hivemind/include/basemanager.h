#pragma once
#include "sc2_forward.h"
#include "baselocation.h"
#include "base.h"
#include "subsystem.h"

namespace hivemind {

  class BaseManager: public Subsystem {
  private:
    BaseVector bases_;
  public:
    BaseManager( Bot* bot );
    void gameBegin() final;
    void gameEnd() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
    BaseVector& bases();
  };

}