#include "stdafx.h"
#include "influence_map.h"
#include "bot.h"
#include "database.h"

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

  bool InfluenceMap::insideMap(int x, int y) const
  {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
  };

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

      auto walkable = [this](int x, int y) -> bool
      {
        return bot_->map().isWalkable(x, y);
      };

      const float range = getUnitRange(unit);
      const int radius = ((range > 0 && range < 4) ? 4 : static_cast<int>(range)) + 2;
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

    /*
    for(auto unit : bot_->observation().GetUnits(sc2::Unit::Alliance::Self))
    {
      markInfluence(unit, -1.0f);
    }
    */

    for(auto unit : bot_->observation().GetUnits(sc2::Unit::Alliance::Enemy))
    {
      markInfluence(unit, 1.0f);
    }
  }

  void InfluenceMap::draw()
  {
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

          string text = std::to_string(int(influence));
          bot_->debug().DebugTextOut(text, pos + sc2::Point3D(0.5f, 0.5f, 0.0f), color);
        }
      }
    }
  }

  bool InfluenceMap::isSafePlace(Vector2 position) const
  {
    auto pos = position;
    pos += sc2::Point3D(0.5f, 0.5f, 0.0f);

    const int x = math::floor(pos.x);
    const int y = math::floor(pos.y);

    bool inside = insideMap(x, y);
    return inside ? influence_.at(y).at(x) == 0 : false;
  }

  Vector2 InfluenceMap::getClosestSafePlace(Vector2 position) const
  {
      auto pos = position;
      pos += sc2::Point3D(0.5f, 0.5f, 0.0f);

      const int startX = math::floor(pos.x);
      const int startY = math::floor(pos.y);

      int bestX = startX;
      int bestY = startY;
      float best = FLT_MAX;

      // Loop in outward spiral until a safe place is found.
      int distance = 1;
      int direction = 0;
      int dx = 0;
      int dy = 0;
      while(distance < 10)
      {
        switch(direction)
        {
          case 0: ++dx; if(dx == distance) ++direction; break;
          case 1: ++dy; if(dy == distance) ++direction; break;
          case 2: --dx; if(-dx == distance) ++direction; break;
          default:
          {
            --dy;
            if(-dy == distance)
            {
              direction = 0;
              ++distance;
            }
            break;
          }
        }
        int x = startX + dx;
        int y = startY + dy;
        bool inside = insideMap(x, y);
        float value = inside ? influence_.at(y).at(x) : FLT_MAX;
        if(value < best)
        {
          best = value;
          bestX = x;
          bestY = y;
        }
        if(value <= 0.0f)
        {
          break;
        }
      }

      return Vector2(bestX * 1.0f, bestY * 1.0f);
  }
}
