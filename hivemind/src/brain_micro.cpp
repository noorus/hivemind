#include "stdafx.h"
#include "brain.h"
#include "ai_goals.h"
#include "bot.h"

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

    void Brain_Micro::activate()
    {
      status_ = AI::Goal::Status_Active;
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

    void Brain_Micro::log(const char* format, const std::string& message)
    {
      string s("Micro: ");
      s += format;
      bot_->console().printf(s.c_str(), message.c_str());
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
    }

    AI::Goal::Status Brain_Micro::process()
    {
      if(status_ == AI::Goal::Status_Inactive)
      {
        return status_;
      }

      sc2::ActionInterface* action = bot_->Actions();

      combatUnits_.updateCenter();

      if(!combatUnits_.focusTarget_)
      {
        combatUnits_.focusTarget_ = selectClosestTarget(combatUnits_.center_);

        if(combatUnits_.focusTarget_)
        {
          log("new target: %s\n", unitPrint(combatUnits_.focusTarget_));
        }
      }

      if(combatUnits_.focusTarget_)
      {
        auto pos = combatUnits_.focusTarget_->pos;
        pos.z += 0.5f;

        bot_->debug().DebugSphereOut(pos, 0.3f, sc2::Colors::White);
      }

      for(auto& entry : combatUnits_.units_)
      {
        const sc2::Unit* unit = entry.first;
        auto& brain = entry.second;

        if(brain.commandCooldown_ > 0)
        {
          auto pos = unit->pos;
          pos.z += 0.3f;

          string text = std::to_string(brain.commandCooldown_);
          bot_->debug().DebugTextOut(text, pos, sc2::Colors::Purple);

          --brain.commandCooldown_;
          continue;
        }

        if(combatUnits_.focusTarget_ && unit->weapon_cooldown == 0)
        {
          // Store move target, if any.
          if(!unit->orders.empty())
          {
            const auto& order = unit->orders.front();
            if(order.ability_id == ABILITY_ID::MOVE)
            {
              brain.moveTarget_ = order.target_pos;
              brain.hasMoveTarget_ = true;
            }
          }

          // Shoot whenever in range.
          const float roachRange = 4.0f;
          if(Distance3D(unit->pos, combatUnits_.focusTarget_->pos) <= roachRange)
          {
            action->UnitCommand(unit, ABILITY_ID::ATTACK, combatUnits_.focusTarget_);
            brain.commandCooldown_ = 0;
          }
        }
        else
        {
          // Continue walking to stored move target, if any.
          if(brain.hasMoveTarget_)
          {
            action->UnitCommand(unit, ABILITY_ID::MOVE, brain.moveTarget_);
            brain.hasMoveTarget_ = false;
            brain.commandCooldown_ = 0;
          }
        }
      }

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

            log("got combat unit: %s\n", unitPrint(unit));
          }
        }
      }
      else if(msg.code == M_Global_UnitDestroyed)
      {
        auto unit = msg.unit();

        if(combatUnits_.units_.erase(unit) > 0)
        {
          log("lost combat unit: %s\n", unitPrint(unit));
        }
        if(unit == combatUnits_.focusTarget_)
        {
          combatUnits_.focusTarget_ = nullptr;
          log("target destroyed: %s\n", unitPrint(unit));
        }
      }
    }

  }
}
