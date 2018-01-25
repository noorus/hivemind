#pragma once
#include "sc2_forward.h"

namespace hivemind {

  class Console;

  class ConsoleListener {
  public:
    virtual void onConsolePrint( Console* console, const string& str ) = 0;
  };

}