#include "stdafx.h"
#include "influence_map.h"
#include "bot.h"

namespace hivemind {


  InfluenceMap::InfluenceMap( Bot* bot ): bot_( bot )
  {
  }

  InfluenceMap::~InfluenceMap()
  {
  }

  void InfluenceMap::gameBegin()
  {
    width_ = bot_->map().width();
    height_ = bot_->map().height();

    influence_.resize(height_, vector<float>(width_, 0.0f));
  }

  void InfluenceMap::update()
  {
    for(auto& row : influence_)
    {
      for(auto& tile : row)
      {
        tile = 0.0f;
      }
    }

    auto markInfluence = [this](auto unit, float influenceChange)
    {
      auto pos = unit->pos;
      pos += sc2::Point3D(0.5f, 0.5f, 0.0f);

      int x = math::floor(pos.x);
      int y = math::floor(pos.y);

      auto insideMap = [this](int x, int y) -> bool
      {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
      };

      auto walkable = [this](int x, int y) -> bool
      {
        return bot_->map().isWalkable(x, y);
      };

      const int radius = 5;
      for(int dy = -radius; dy <= radius; ++dy)
      {
        for(int dx = -radius; dx <= radius; ++dx)
        {
          if(dx * dx + dy * dy > radius * radius)
          {
            continue;
          }

          if(insideMap(x + dx, y + dy) && walkable(x + dx, y + dy))
          {
            influence_.at(y + dy).at(x + dx) += influenceChange;
          }
        }
      }
    };

    for(auto unit : bot_->observation().GetUnits(sc2::Unit::Alliance::Self))
    {
      markInfluence(unit, 1.0f);
    }

    for(auto unit : bot_->observation().GetUnits(sc2::Unit::Alliance::Enemy))
    {
      markInfluence(unit, -1.0f);
    }
  }

  void InfluenceMap::draw()
  {
    auto to_string = [](int x) -> string
    {
      char buf[40];
      sprintf_s(buf, "%d", x);
      return buf;
    };

    for(size_t y = 0; y < height_; ++y)
    {
      for(size_t x = 0; x < width_; ++x)
      {
        float influence = influence_.at(y).at(x);

        if(influence != 0.0f)
        {
          sc2::Color color = influence > 0.0f ? sc2::Color(255, 0, 0) : sc2::Color(0, 0, 255);

          auto pos = sc2::Point3D(float(x), float(y), 0.0f);
          pos.z = bot_->map().heightMap_[x][y] + 0.0f;

          bot_->debug().DebugBoxOut(pos, pos + sc2::Point3D(1.0f, 1.0f, 0.0f), color);

          string text = to_string(int(influence));
          bot_->debug().DebugTextOut(text, pos + sc2::Point3D(0.5f, 0.5f, 0.0f), color);
        }
      }
    }
  }

}
