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
    BuildPlacement_MainBuilding, //!< Special placement for main buildings
    BuildPlacement_Extractor, //!< Special placement for gas extractors
    BuildPlacement_Front, //!< Weighted toward front of the base
    BuildPlacement_Back, //!< Weighted toward back of the base
    BuildPlacement_MineralLine, //!< In the mineral line
    BuildPlacement_Choke, //!< At the choke/ramp
    BuildPlacement_Hidden //!< As stealthy as possible
  };

  using BuildProjectID = uint64_t;

  struct Building {
    BuildProjectID id;
    UnitTypeID type;
    UnitRef building;
    UnitRef builder;
    UnitRef target; //!< Targets unit instead of position; overrides position if not null
    MapPoint2 position;
    bool cancel;
    GameTime nextUpdateTime;
    GameTime buildStartTime;
    GameTime buildCompleteTime;
    GameTime lastOrderTime;
    bool completed;
    bool moneyAllocated;
    size_t tries;
    size_t orderTries;
    AbilityID buildAbility;

    Building(BuildProjectID id_, UnitTypeID type_, AbilityID abil_) :
        id(id_),
        type(type_),
        building(nullptr),
        builder(nullptr),
        cancel(false),
        position(0, 0),
        nextUpdateTime(0),
        tries(0),
        orderTries(0),
        buildAbility(abil_),
        buildStartTime(0),
        buildCompleteTime(0),
        completed(false),
        moneyAllocated(false),
        lastOrderTime(0),
      target( nullptr )
    {
    }
  };

  using BuildingVector = vector<Building>;

  class Builder: public Subsystem, public hivemind::Listener {
  protected:
    BuildingVector buildProjects_;
    BuildProjectID idPool_;
    virtual void onMessage( const Message& msg ) final;
  public:
    Builder( Bot* bot );
    void gameBegin() final;
    bool add( UnitTypeID structure, const Base& base, BuildingPlacement placement, BuildProjectID& idOut );
    void remove( BuildProjectID id );
    void update( const GameTime time, const GameTime delta );
    void draw() override;
    void gameEnd() final;
    bool findPlacement( UnitTypeID structure, const Base& base, BuildingPlacement type, AbilityID ability, Vector2& placementOut, UnitRef& targetOut );

    // Returns the amount of {minerals,vespene} that the not-yet-paid-trainings will cost.
    std::pair<int,int> getAllocatedResources() const;
  };

}