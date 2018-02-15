#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "distancemap.h"
#include "hive_rect2.h"
#include "base.h"
#include "trainer.h"

namespace hivemind {

  class Bot;
  enum class UpgradeStatus;

  using BuildProjectID = uint64_t;

  struct Research {
    BuildProjectID id;
    UpgradeID upgradeType;
    UnitTypeID researcherType;
    UnitRef researcher;
    bool cancel;
    GameTime nextUpdateTime;
    GameTime buildStartTime;
    GameTime buildCompleteTime;
    GameTime lastOrderTime;
    bool completed;
    bool moneyAllocated;
    size_t tries;
    size_t orderTries;

    Research(BuildProjectID id_, UpgradeID upgradeType_, UnitTypeID researcherType_, UnitRef researcher_) :
        id(id_),
        upgradeType(upgradeType_),
        researcherType(researcherType_),
        researcher(researcher_),
        cancel(false),
        nextUpdateTime(0),
        tries(0),
        orderTries(0),
        buildStartTime(0),
        buildCompleteTime(0),
        completed(false),
        moneyAllocated(false),
        lastOrderTime(0)
    {
    }
  };

  using ResearchVector = vector<Research>;

  class Researcher: public Subsystem, public hivemind::Listener {
  protected:

    virtual void onMessage( const Message& msg ) final;

    ResearchVector researchProjects_;
    BuildProjectID& idPool_;
    UnitSet researchers_;
    std::unordered_map<sc2::UPGRADE_ID, UpgradeStatus>& upgradeStats_;

    UnitRef getResearcher(Base& base, UnitTypeID researcherType) const;

  public:
    Researcher( Bot* bot, BuildProjectID& idPool, std::unordered_map<sc2::UPGRADE_ID, UpgradeStatus>& upgradeStats );

    bool research( UpgradeID upgradeType, Base* base, UnitTypeID researcherType, BuildProjectID& idOut );

    void remove( BuildProjectID id );

    void update( const GameTime time, const GameTime delta );

    void gameBegin() final;
    void gameEnd() final;

    // Returns the amount of resources that the not-yet-paid-upgrades will cost.
    AllocatedResources getAllocatedResources() const;
  };

}
