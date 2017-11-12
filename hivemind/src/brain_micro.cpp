#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"

namespace hivemind {

  static string unitPrint(UnitRef unit)
  {
    char text[200];
    sprintf_s(text, "%s at (%.2f, %.2f, %.2f)", sc2::UnitTypeToName(unit->unit_type), unit->pos.x, unit->pos.y, unit->pos.z);
    return text;
  }

  namespace Goals {

    Brain_Micro::Brain_Micro( AI::Agent * agent ):
      AI::CompositeGoal( agent )
    {
      bot_->messaging().listen( Listen_Global | Listen_Intel, this );

      activate();
    }

    static Vector2 getNextSquadMoveTarget()
    {
      static int x = 0;
      ++x;

      if(x == 1)
      {
        return Vector2(15.0f, 25.0f); // Left side of the map.
      }
      else if(x == 2)
      {
        return Vector2(33.0f, 25.0f); // Center of the map.
      }
      else
      {
        float x = utils::randomBetween(15, 45) * 1.0f;
        float y = utils::randomBetween(15, 45) * 1.0f;
        return Vector2(x, y);
      }
    }

    void Brain_Micro::activate()
    {
      status_ = AI::Goal::Status_Active;

      combatUnits_.moveTarget_ = getNextSquadMoveTarget();
    }

    void Brain_Micro::drawSquad(Squad& squad)
    {
      float z = 8.1f;
      Vector3 center(squad.center_.x, squad.center_.y, z);
      Vector3 moveTarget(squad.moveTarget_.x, squad.moveTarget_.y, z);

      if(squad.focusTarget_)
      {
        auto focusTarget = squad.focusTarget_->pos;
        focusTarget.z += 0.1f;

        bot_->debug().DebugLineOut(center, focusTarget, sc2::Colors::Red);
      }

      bot_->debug().DebugSphereOut(center, squad.radius_);
      bot_->debug().DebugLineOut(center, moveTarget);
    }

    void Brain_Micro::drawSquadUnits(Squad& squad)
    {
      for(auto& entry : squad.units_)
      {
        UnitRef unit = entry.first;
        const auto& brain = entry.second;

        auto pos = unit->pos;

        if(brain.hasMoveTarget_)
        {
          float z = 8.1f;
          Vector3 center(pos.x, pos.y, z);
          Vector3 moveTarget(brain.moveTarget_.x, brain.moveTarget_.y, z);

          bot_->debug().DebugLineOut(center, moveTarget, sc2::Colors::Green);
        }

        pos.z += 0.3f;
        string text = std::to_string(brain.commandCooldown_);
        bot_->debug().DebugTextOut(text, pos, sc2::Colors::Purple);

        pos.y += 0.1f;
        pos.z += 0.1f;
        string text2 = std::to_string(unit->weapon_cooldown);
        bot_->debug().DebugTextOut(text2, pos, sc2::Colors::Yellow);
      }
    }

    void Brain_Micro::updateSquad(Squad& squad)
    {
      squad.updateCenter();

      if(squad.center_.distance(squad.moveTarget_) <= 5.0f)
      {
        Vector2 newTarget = getNextSquadMoveTarget();
        log("squad at move target (%f, %f), moving to (%f, %f)", squad.moveTarget_.x, squad.moveTarget_.y, newTarget.x, newTarget.y);

        squad.moveTarget_ = newTarget;
      }

      if(!squad.focusTarget_)
      {
        squad.focusTarget_ = selectClosestTarget(squad.center_);

        if(squad.focusTarget_)
        {
          log("new squad focus target: %s", unitPrint(squad.focusTarget_));
        }
      }
    }

    void Brain_Micro::updateSquadUnits(Squad& squad)
    {
      for(auto& entry : squad.units_)
      {
        updateSquadUnit(squad, entry.first, entry.second);
      }
    }

    void Brain_Micro::updateSquadUnit(Squad& squad, UnitRef unit, UnitBrain& brain)
    {
      sc2::ActionInterface* action = bot_->Actions();

      if(brain.commandCooldown_ > 0)
      {
        // Don't interrupt previous attack command.
        --brain.commandCooldown_;
        return;
      }

      brain.hasMoveTarget_ = false;

      if(squad.focusTarget_ && unit->weapon_cooldown <= 0)
      {
        // Ready to attack.

        auto target = selectTarget(squad, unit);
        if(target)
        {
          // Shoot if something is in range.

          action->UnitCommand(unit, ABILITY_ID::ATTACK, target);
          float damagePoint = getUnitDamagePoint(unit);
          brain.commandCooldown_ = static_cast<int>(damagePoint * 1000 / 10);
        }
        else
        {
          // Move closer to shoot if nothing is in range.

          action->UnitCommand(unit, ABILITY_ID::ATTACK, squad.focusTarget_);
          brain.commandCooldown_ = 0;
        }
      }
      else
      {
        bool safe = bot_->influenceMap().isSafePlace(unit->pos);

        if(safe)
        {
          auto target = squad.moveTarget_;
          action->UnitCommand(unit, ABILITY_ID::MOVE, target);
        }
        else
        {
          auto target = getKitingPosition(unit);
          action->UnitCommand(unit, ABILITY_ID::MOVE, target);
          brain.moveTarget_ = target;
          brain.hasMoveTarget_ = true;
          //brain.commandCooldown_ = 100;
        }
      }
    }

    Vector2 Brain_Micro::getKitingPosition(UnitRef unit)
    {
      return bot_->influenceMap().getClosestSafePlace(unit->pos);
    }

    UnitRef Brain_Micro::selectClosestTarget(Vector2 from)
    {
      const sc2::ObservationInterface* observation = bot_->Observation();
      sc2::Units units = observation->GetUnits(Unit::Alliance::Enemy);

      if (units.empty())
      {
        return false;
      }

      UnitRef target = nullptr;
      float distance = std::numeric_limits<float>::max();
      for(const auto& unit : units)
      {
        float d = DistanceSquared2D(unit->pos, from);

        if(utils::isBuilding(unit))
        {
          const float buildingDistancePenalty = 1000.0f;
          d += buildingDistancePenalty;
        }

        if(d < distance)
        {
          distance = d;
          target = unit;
        }
      }

      return target;
    }

    UnitRef Brain_Micro::selectWeakestTarget(Vector2 from, float range)
    {
      const sc2::ObservationInterface* observation = bot_->Observation();
      sc2::Units units = observation->GetUnits(Unit::Alliance::Enemy);

      if (units.empty())
      {
        return false;
      }

      UnitRef target = nullptr;
      float distance = range;
      float health = std::numeric_limits<float>::max();
      for(const auto& unit : units)
      {
        float d = DistanceSquared2D(unit->pos, from);

        if(d <= distance * distance && unit->health <= health)
        {
          distance = d;
          target = unit;
          health = unit->health;
        }
      }

      return target;
    }

    void Brain_Micro::log(const char* format, const std::string& message)
    {
      string s("Micro: ");
      s += format;
      bot_->console().printf(s.c_str(), message.c_str());
    }

    template<typename ...Args>
    void Brain_Micro::log(const char* format, Args... args)
    {
      string s("Micro: ");
      s += format;
      bot_->console().printf(s.c_str(), args...);
    }

    void Brain_Micro::Squad::updateCenter()
    {
      Vector2 sum;
      for(const auto& entry : units_)
      {
        sum.x += entry.first->pos.x;
        sum.y += entry.first->pos.y;
      }
      center_ = sum / float(units_.size());

      float maxDistance = 0.0f;
      for(const auto& entry : units_)
      {
        float x = entry.first->pos.x;
        float y = entry.first->pos.y;
        Vector2 pos(x, y);

        float r = center_.distance(pos);
        maxDistance = max(r, maxDistance);
      }
      radius_ = maxDistance;
    }

    UnitRef Brain_Micro::selectTarget(const Squad& squad, UnitRef unit)
    {
      const float range = getUnitRange(unit);

      UnitRef weakest = selectWeakestTarget(unit->pos, range);

      if(weakest)
      {
        float hp = weakest->health * 1.0f / weakest->health_max;
        if(hp < 0.3f)
        {
          return weakest;
        }
      }

      // Prefer squad focus target if in range.
      if(Distance3D(unit->pos, squad.focusTarget_->pos) <= range)
      {
        return squad.focusTarget_;
      }

      // Otherwise attack the weakest in range.
      return weakest;
    }

    AI::Goal::Status Brain_Micro::process()
    {
      if(status_ == AI::Goal::Status_Inactive)
      {
        return status_;
      }

      updateSquad(combatUnits_);
      updateSquadUnits(combatUnits_);
      drawSquad(combatUnits_);
      drawSquadUnits(combatUnits_);

      return status_;
    }

    void Brain_Micro::terminate()
    {
      status_ = AI::Goal::Status_Inactive;
    }

    void Brain_Micro::onMessage( const Message & msg )
    {
      if (msg.code == M_Global_UnitCreated )
      {
        auto unit = msg.unit();

        if(utils::isMine(unit))
        {
          if(!utils::isWorker(unit) &&
            !utils::isBuilding(unit) &&
            unit->unit_type.ToType() != sc2::UNIT_TYPEID::ZERG_QUEEN &&
            unit->unit_type.ToType() != sc2::UNIT_TYPEID::ZERG_OVERLORD &&
            unit->unit_type.ToType() != sc2::UNIT_TYPEID::ZERG_LARVA &&
            unit->unit_type.ToType() != sc2::UNIT_TYPEID::ZERG_CREEPTUMORQUEEN)
          {
            combatUnits_.units_[unit] = UnitBrain();

            log("got combat unit: %s", unitPrint(unit));
          }
        }
      }
      else if(msg.code == M_Global_UnitDestroyed)
      {
        auto unit = msg.unit();

        if(combatUnits_.units_.erase(unit) > 0)
        {
          log("lost combat unit: %s", unitPrint(unit));
        }
        if(unit == combatUnits_.focusTarget_)
        {
          combatUnits_.focusTarget_ = nullptr;
          log("target destroyed: %s", unitPrint(unit));
        }
      }
    }

  }
}
