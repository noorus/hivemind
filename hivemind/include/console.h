#pragma once
#include "sc2_forward.h"
#include "subsystem.h"

namespace hivemind {

  class TextFile {
  protected:
    HANDLE file_;
  public:
    TextFile( const string& filename );
    void write( const string& str );
    ~TextFile();
  };

  class Console: public Subsystem {
  private:
    TextFile* fileOut_;
    void writeStartBanner();
    void writeStopBanner();
  public:
    Console( Bot* bot );
    void gameBegin() final;
    void gameEnd() final;
    void printf( const char* str, ... );
  };

}