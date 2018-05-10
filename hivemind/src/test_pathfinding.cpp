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
      virtual bool isBlocked(int x, int y) const
      {
        return (grid_.at(y).at(x) & Wall) == Blocked;
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

    static void plant(const DStarLitePtr& g, TestGridMap* rawM, int w, int h, int x, int y, bool add)
    {
      for(int i = 0; i < w; ++i)
      {
        for(int j = 0; j < h; ++j)
        {
          auto z = MapPoint2{ x, y };
          z.x += i;
          z.y += j;
          g->updateWalkability(z, add);
          if(add)
            rawM->grid_.at(z.y).at(z.x) |= TestGridMap::Blocked;
          else
            rawM->grid_.at(z.y).at(z.x) &= ~TestGridMap::Blocked;
        }
      }

      g->computeShortestPath();
    }

    static void testPathfinding1(Console* console)
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

      auto path1 = g->getMapPath();
      assert(path1.size() == 32);

      plant(g, rawM, 1, 1, 13, 4, true);

      auto path2 = g->getMapPath();
      assert(path2.size() == 33);

      plant(g, rawM, 1, 1, 14, 4, true);

      auto path3 = g->getMapPath();
      assert(path3.size() == 34);
    }

    static void testPathfinding2(Console* console)
    {
      string s =
        "....#....\n"
        "....#....\n"
        "....#....\n"
        "....#....\n"
        "....#....\n"
        ".........";

      auto start = MapPoint2(3, 0);
      auto goal = MapPoint2(7, 0);

      auto m = std::make_unique<TestGridMap>(s);
      auto* rawM = m.get();
      auto g = pathfinding::DStarLite::search(console, std::move(m), start, goal);

      auto path1 = g->getMapPath();
      assert(path1.size() == 11);

      plant(g, rawM, 2, 2, 2, 3, true);

      auto path2 = g->getMapPath();
      assert(path2.size() == 13);

      plant(g, rawM, 2, 2, 0, 3, true);

      auto path3 = g->getMapPath();
      assert(path3.size() == 11);
    }

    static void testPathfinding3(Console* console)
    {
      string s =
        ".........\n"
        ".........\n"
        ".........\n"
        ".........";

      auto start = MapPoint2(1, 1);
      auto goal = MapPoint2(7, 1);

      auto m = std::make_unique<TestGridMap>(s);
      auto* rawM = m.get();
      auto g = pathfinding::DStarLite::search(console, std::move(m), start, goal);

      auto path1 = g->getMapPath();
      assert(path1.size() == 7);

      plant(g, rawM, 2, 3, 1, 0, true);

      auto path2 = g->getMapPath();
      assert(path2.size() == 9);

      plant(g, rawM, 2, 3, 1, 0, false);

      auto path3 = g->getMapPath();
      assert(path3.size() == 7);

      plant(g, rawM, 2, 3, 6, 0, true);

      rawM->print(console);

      auto path4 = g->getMapPath();
      assert(path4.size() == 9);

      plant(g, rawM, 2, 3, 6, 0, false);
      auto path5 = g->getMapPath();
      assert(path5.size() == 7);
    }

    void testPathfinding(Console* console)
    {
      testPathfinding3(console);
      testPathfinding2(console);
      testPathfinding1(console);
    }
  }
}