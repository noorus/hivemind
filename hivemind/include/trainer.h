#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "distancemap.h"
#include "hive_rect2.h"
#include "base.h"

namespace hivemind {

  class Bot;

  using BuildProjectID = uint64_t;

  class UnitStats;

  struct AllocatedResources
  {
    int minerals;
    int vespene;
    int food;
  };

  struct Training {
    BuildProjectID id;
    UnitTypeID type;
    UnitTypeID trainerType;
    UnitRef trainer;
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

    Training(BuildProjectID id_, UnitTypeID type_, UnitTypeID trainerType_, UnitRef trainer_) :
        id(id_),
        type(type_),
        trainerType(trainerType_),
        trainer(trainer_),
        cancel(false),
        position(0, 0),
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

  using TrainingVector = vector<Training>;

  class Trainer: public Subsystem, public hivemind::Listener {
  protected:
    TrainingVector trainingProjects_;
    virtual void onMessage( const Message& msg ) final;
    BuildProjectID& idPool_;
    std::unordered_map<sc2::UNIT_TYPEID, UnitStats>& unitStats_;
    UnitSet trainers_;

    UnitRef getTrainer(Base& base, UnitTypeID trainerType) const;

    void onTrainingComplete(const Training& training);

  public:
    Trainer( Bot* bot, BuildProjectID& idPool, std::unordered_map<sc2::UNIT_TYPEID, UnitStats>& );
    void gameBegin() final;
    bool train( UnitTypeID unitType, Base* base, UnitTypeID trainerType, BuildProjectID& idOut );
    void remove( BuildProjectID id );
    void update( const GameTime time, const GameTime delta );
    void gameEnd() final;

    // Returns the amount of resources that the not-yet-paid-trainings will cost.
    AllocatedResources getAllocatedResources() const;
  };

}
