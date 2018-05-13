#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "database.h"
#include "map.h"
#include "bot.h"
#include "console.h"

namespace hivemind {

  class ControllerBase {
  protected:
    static ActionInterface* actions_;
  public:
    static void setActions( ActionInterface* iface )
    {
      actions_ = iface;
    }
  };

  template <class T>
  class Controller: public ControllerBase {
  private:
    Controller() = default;
  protected:
    UnitRef unit_;
  public:
    Controller( UnitRef unit ): unit_( unit ) {}
    //! Generic: Move to position
    T& move( const Vector2& position )
    {
      actions_->UnitCommand( unit_, sc2::ABILITY_ID::MOVE, position, false );
      return *(T*)this;
    }
    //! Generic: Stop whatever you're doing
    T& stop()
    {
      actions_->UnitCommand( unit_, sc2::ABILITY_ID::STOP, false );
      return *(T*)this;
    }
  };

# define CONTROLLER_CONSTRUCTOR( className )\
  static const string& name() { return #className; }\
  ##className##( UnitRef unit ): Controller( unit )

  class Overlord: public Controller<Overlord> {
  public:
    CONTROLLER_CONSTRUCTOR( Overlord ) {}
  };

  class Drone: public Controller<Drone> {
  public:
    CONTROLLER_CONSTRUCTOR( Drone ) {}
    //! Drone: Build structure on target unit (extractor) (ability)
    Drone& build( AbilityID ability, UnitRef target )
    {
      actions_->UnitCommand( unit_, ability, target, false );
      return *this;
    }
    //! Drone: Build structure at location (ability)
    Drone& build( AbilityID ability, const Vector2& target )
    {
      actions_->UnitCommand( unit_, ability, target, false );
      if ( g_Bot )
        g_Bot->console().printf( "DEBUG: Drone - Giving build command for %s at (%.2f,%.2f)", sc2::AbilityTypeToName( ability ), target.x, target.y );
      return *this;
    }
    //! Drone: Start gathering minerals or gas from target unit
    Drone& harvestGather( UnitRef target )
    {
      actions_->UnitCommand( unit_, sc2::ABILITY_ID::HARVEST_GATHER, target );
      return *this;
    }
  };

  class Larva: public Controller<Larva> {
  public:
    CONTROLLER_CONSTRUCTOR( Larva ) {}
    //! Larva: Morph to unit (ability)
    Larva& morph( UnitTypeID to )
    {
      auto ability = Database::techTree().getBuildAbility( to, unit_->unit_type ).ability;
      actions_->UnitCommand( unit_, ability );
      return *this;
    }
  };

  class Queen : public Controller<Queen> {
  public:
    CONTROLLER_CONSTRUCTOR( Queen ) {}
    //! Queen: Spawn creep tumor (ability)
    Queen& tumor( const Vector2& target )
    {
      actions_->UnitCommand( unit_, sc2::ABILITY_ID::BUILD_CREEPTUMOR_QUEEN, target, false );
      return *this;
    }
    //! Queen: Inject larvae (ability)
    Queen& inject( UnitRef hatchery )
    {
      actions_->UnitCommand( unit_, sc2::ABILITY_ID::EFFECT_INJECTLARVA, hatchery, false );
      return *this;
    }
  };

  class CreepTumor : public Controller<CreepTumor>
  {
  public:
    CONTROLLER_CONSTRUCTOR( CreepTumor ) {}
    //! CreepTumor: Spawn next creep tumor (ability)
    CreepTumor& tumor( const Vector2& target )
    {
      actions_->UnitCommand( unit_, sc2::ABILITY_ID::BUILD_CREEPTUMOR_TUMOR, target, false );
      return *this;
    }
  };

}
