#pragma once

namespace hivemind {

  class Bot;

  class Subsystem {
  public:
    Bot* bot_;
  public:
    Subsystem( Bot* bot ): bot_( bot ) {}
    virtual void gameBegin() = 0;
    virtual void gameEnd() = 0;
    virtual void draw() {}
  };

}