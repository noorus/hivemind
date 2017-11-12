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

    bool isSafePlace(Vector2 position) const;
    Vector2 getClosestSafePlace(Vector2 position) const;

  private:

    bool insideMap(int x, int y) const;

    vector<vector<float>> influence_;
    size_t width_;
    size_t height_;
  };

}
