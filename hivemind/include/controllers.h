#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "database.h"
#include "map.h"

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
    T& move( const Vector2& position )
    {
      actions_->UnitCommand( unit_, sc2::ABILITY_ID::MOVE, position, false );
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
    Drone& build( AbilityID ability, UnitRef target )
    {
      actions_->UnitCommand( unit_, ability, target, false );
      return *this;
    }
    Drone& build( AbilityID ability, const MapPoint2& target )
    {
      actions_->UnitCommand( unit_, ability, target, false );
      return *this;
    }
  };

  class Larva: public Controller<Larva> {
  public:
    CONTROLLER_CONSTRUCTOR( Larva ) {}
    Larva& morph( UnitTypeID to )
    {
      auto ability = Database::techTree().getBuildAbility( to, unit_->unit_type ).ability;
      actions_->UnitCommand( unit_, ability );
      return *this;
    }
  };

}
