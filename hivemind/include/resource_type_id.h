#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "ai_goals.h"
#include "ai_agent.h"
#include "messaging.h"
#include "hive_vector2.h"

namespace hivemind {

  // A ResourceTypeID is either a UNIT_TYPEID or a UPGRADE_ID, or empty.
  struct ResourceTypeID
  {
    ResourceTypeID()
    {
      unitType_ = UNIT_TYPEID::INVALID;
      upgradeType_ = UPGRADE_ID::INVALID;
      name_ = "<null>";
    }

    ResourceTypeID(UNIT_TYPEID unitType)
    {
      unitType_ = unitType;
      upgradeType_ = UPGRADE_ID::INVALID;
      name_ = UnitTypeToName(unitType);
    }

    ResourceTypeID(UPGRADE_ID upgradeType)
    {
      unitType_ = UNIT_TYPEID::INVALID;
      upgradeType_ = upgradeType;
      name_ = UpgradeIDToName(upgradeType);
    }

    bool isUnitType() const { return unitType_ != UNIT_TYPEID::INVALID; }
    bool isUpgradeType() const { return upgradeType_ != UPGRADE_ID::INVALID; }
    bool isValid() const { return isUnitType() || isUpgradeType(); }

    const char* name_;
    UnitTypeID unitType_;
    UpgradeID upgradeType_;
  };

  inline bool operator==(ResourceTypeID lhs, ResourceTypeID rhs)
  {
    return std::tie(lhs.unitType_, lhs.upgradeType_) == std::tie(rhs.unitType_, rhs.upgradeType_);
  }

  inline bool operator!=(ResourceTypeID lhs, ResourceTypeID rhs)
  {
    return !(lhs == rhs);
  }
}
