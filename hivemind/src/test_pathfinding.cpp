#include "stdafx.h"

#include "pathfinding.h"
#include "console.h"

namespace hivemind {

  namespace pathfinding {

    class TestGridMap: public pathfinding::GridMap
    {
    public:

      enum State
      {
        Empty = 0,
        Wall = 1,
        Blocked = 2
      };

      int width_;
      int height_;
      vector< vector<int> > grid_;

      explicit TestGridMap(string s):
        width_(0),
        height_(1)
      {
        grid_.push_back(vector<int>());

        for(char c : s)
        {
          if(c == '\n')
          {
            grid_.push_back(vector<int>());
            ++height_;
          }
          else if(c == '.')
          {
            grid_.back().push_back(Empty);
          }
          else if(c == '#')
          {
            grid_.back().push_back(Wall);
          }
          else
          {
            grid_.back().push_back(Blocked);
          }
        }
        width_ = (int)grid_.back().size();
      }

      virtual int width() const override
      {
        return width_;
      }
      virtual int height() const override
      {
        return height_;
      }
      virtual bool isWalkable(int x, int y) const
      {
        return (grid_.at(y).at(x) & Wall) != Wall;
      }

      void print(Console* console)
      {
        for(auto& r : grid_)
        {
          string s;
          for(auto& c : r)
          {
            s += std::to_string(int(c));
          }
          console->printf("%s", s.c_str());
        }
      }
    };

    void testPathfinding(Console* console)
    {
      string s =
        "...............#\n"
        "###############.\n"
        "###############.\n"
        "##############..\n"
        "#############...\n"
        "...............#";

      auto start = MapPoint2(0, 0);
      auto goal = MapPoint2(0, 5);

      auto m = std::make_unique<TestGridMap>(s);
      auto* rawM = m.get();
      auto g = pathfinding::DStarLite::search(console, std::move(m), start, goal);

      auto plant = [&g, rawM](int w, int h, int x, int y)
      {
        for(int i = 0; i < w; ++i)
          for(int j = 0; j < h; ++j)
          {
            auto z = MapPoint2{x,y};
            z.x += i;
            z.y += j;
            g->updateWalkability(z, true);
            rawM->grid_.at(z.y).at(z.x) |= TestGridMap::Blocked;
          }
      };

      plant(2, 1, 13, 4);

      //rawM->print(console);

      auto path = g->getMapPath();
      assert(path.size() == 34);
    }
  }

}
