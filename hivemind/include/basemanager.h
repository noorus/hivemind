#pragma once
#include "sc2_forward.h"
#include "baselocation.h"
#include "base.h"
#include "subsystem.h"
#include "messaging.h"
#include "hive_vector3.h"

namespace hivemind {

  class BaseManager: public Subsystem, private Listener {
    friend class Base;
  private:
    BaseVector bases_;
    UnitSet depots_;
    UnitSet assignedUnits_;
    void onMessage( const Message& msg ) final;
    void debugSpawnQueen( UnitRef depot );
  public:
    BaseManager( Bot* bot );
    bool addBase( UnitRef depot );
    void gameBegin() final;
    void gameEnd() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
    Base* findClosest( const Vector3& pos );
    void addBuilding( UnitRef unit );
    void addWorker( UnitRef unit );
    void removeWorker( UnitRef unit );
    void removeLarva( UnitRef unit );
    BaseVector& bases();
  };

}