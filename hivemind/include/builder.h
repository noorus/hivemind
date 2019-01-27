#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "distancemap.h"
#include "hive_rect2.h"
#include "base.h"
#include "trainer.h"
#include "researcher.h"

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

  class UnitStats
  {
  public:
    int unitCount_;
    int inProgressCount_;

    int inProgressCount() const
    {
      return inProgressCount_;
    }

    int futureCount() const
    {
      return unitCount() + inProgressCount();
    }

    int unitCount() const
    {
      return unitCount_;
    }
  };

  class BuilderUnitStats
  {
  public:
    UnitSet units;
    std::set<BuildProjectID> inProgress;

    int inProgressCount() const
    {
      return static_cast<int>(inProgress.size());
    }

    int futureCount() const
    {
      return unitCount() + inProgressCount();
    }

    int unitCount() const
    {
      return static_cast<int>(units.size());
    }
  };

  enum class UpgradeStatus
  {
    NotStarted,
    inProgress,
    Researched
  };

  struct Building {
    BuildProjectID id;
    Base* base;
    UnitTypeID type;
    UnitRef building;
    UnitRef builder;
    UnitRef target; //!< Targets unit instead of position; overrides position if not null
    Vector2 position;
    MapPoint2 footprintPosition;
    bool cancel;
    GameTime nextUpdateTime;
    GameTime buildStartTime;
    GameTime buildCompleteTime;
    GameTime lastOrderTime;
    bool completed;
    bool moneyAllocated;
    size_t workerTries;
    size_t orderTries;
    AbilityID buildAbility;

    Building(BuildProjectID id_, Base* base_, UnitTypeID type_, AbilityID ability_) :
        id(id_),
        base(base_),
        type(type_),
        building(nullptr),
        builder(nullptr),
        cancel(false),
        position(0, 0),
        nextUpdateTime(0),
        workerTries(0),
        orderTries(0),
        buildAbility(ability_),
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

    Trainer trainer_;
    Researcher researcher_;
    friend Trainer;
    friend Researcher;

  private:
    std::unordered_map<sc2::UNIT_TYPEID, BuilderUnitStats> unitStats_;
    std::unordered_map<sc2::UPGRADE_ID, UpgradeStatus> upgradeStats_;

    UnitRef acquireBuilder(Base& base);

    std::unordered_map<BuildProjectID, int> finishedBuildProjects_;

  public:
    explicit Builder( Bot* bot );
    void gameBegin() final;

    bool build( UnitTypeID structureType, Base* base, BuildingPlacement placement, BuildProjectID& idOut );
    bool train( UnitTypeID unitType, Base* base, BuildProjectID& idOut );
    bool research( UpgradeID upgradeType, Base* base, UnitTypeID researcherType, BuildProjectID& idOut );

    bool make( UnitTypeID unitType, Base* base, BuildProjectID& idOut );

    void remove( BuildProjectID id );
    void update( const GameTime time, const GameTime delta );
    void draw() override;
    void gameEnd() final;
    bool findPlacement( UnitTypeID structure, const Base& base, BuildingPlacement type, AbilityID ability, Vector2& placementOut, UnitRef& targetOut );

    // Returns the amount of resources that the not-yet-paid-trainings will cost.
    AllocatedResources getAllocatedResources() const;

    UnitStats getUnitStats(UnitTypeID unitType);

    UnitStats getUpgradeStatus(sc2::UPGRADE_ID upgradeType)
    {
      auto stats = upgradeStats_[upgradeType];
      return { stats == UpgradeStatus::Researched, stats == UpgradeStatus::inProgress };
    }

    bool haveResourcesToMake(UnitTypeID unitType, AllocatedResources allocatedResources) const;
    bool haveResourcesToMake(UpgradeID upgradeType, AllocatedResources allocatedResources) const;
    bool haveResourcesToMake(AllocatedResources cost, AllocatedResources allocatedResources) const;

    static AllocatedResources getCost(UnitTypeID unitType);
    AllocatedResources getCost(UpgradeID upgradeType) const;

    bool isFinished(BuildProjectID id) const
    {
      return finishedBuildProjects_.find(id) != finishedBuildProjects_.end();
    }
  };

  UnitTypeID getTrainerType(UnitTypeID unitType);
  UnitTypeID getResearcherType(UpgradeID upgradeType);

}
