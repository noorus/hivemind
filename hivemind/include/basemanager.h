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
    TagSet depots_;
    TagSet assignedUnits_;
    void onMessage( const Message& msg ) final;
  public:
    BaseManager( Bot* bot );
    bool addBase( const Unit& depot );
    void gameBegin() final;
    void gameEnd() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
    Base* findClosest( const Vector3& pos );
    void addBuilding( const Unit& unit );
    void addWorker( const Unit& unit );
    void removeWorker( const Tag unit );
    BaseVector& bases();
  };

}