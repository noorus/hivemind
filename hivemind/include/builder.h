#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "distancemap.h"
#include "hive_rect2.h"
#include "base.h"

namespace hivemind {

  class Bot;

  enum BuildingPlacement {
    BuildPlacement_Generic, //!< Growing outwards from main building
    BuildPlacement_Front, //!< Weighted toward front of the base
    BuildPlacement_Back, //!< Weighted toward back of the base
    BuildPlacement_MineralLine, //!< In the mineral line
    BuildPlacement_Choke, //!< At the choke/ramp
    BuildPlacement_Hidden //!< As stealthy as possible
  };

  class Builder: public Subsystem {
  public:
    Builder( Bot* bot );
    void gameBegin() final;
    void gameEnd() final;
    Point2D findPlacement( UnitTypeID structure, const Base& base, BuildingPlacement type );
  };

}