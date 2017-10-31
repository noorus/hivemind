#pragma once
#include "sc2_forward.h"
#include "map.h"

namespace hivemind {

  class Bot;

  class InfluenceMap {
  public:
    Bot* bot_;

  public:
    InfluenceMap( Bot* bot );
    ~InfluenceMap();
    void draw();
    void update();
    void gameBegin();

  private:
    vector<vector<float>> influence_;
    size_t width_;
    size_t height_;
  };

}
