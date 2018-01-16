#pragma once
#include "sc2_forward.h"
#include "messaging.h"

namespace hivemind {

  class WorkerManager: public Subsystem, private Listener {
  private:
    UnitSet ignored_; //! Workers that have been released for other use and not managed by us
    UnitSet workers_;
    UnitSet idle_;
    void _created( UnitRef unit );
    void _destroyed( UnitRef unit );
    void _idle( UnitRef unit );
    void _onIdle( UnitRef worker );
    void _onActivate( UnitRef worker );
    void _refreshWorkers();
  private:
    void onMessage( const Message& msg ) final;
  public:
    WorkerManager( Bot* bot );
    virtual void gameBegin() override;
    virtual void gameEnd() override;
    const UnitSet& all() const;
    bool add( UnitRef worker );
    bool addBack( UnitRef worker ); //!< Return a previously released worker
    const bool exists( UnitRef worker ) const;
    const bool ignored( UnitRef worker ) const;
    const bool idle( UnitRef worker ) const;
    void remove( UnitRef worker );
    UnitRef release(); //! Release a worker for other use
    UnitRef releaseClosest( const Vector2& to );
    virtual void draw() override;
    void update( GameTime time );
    void initialise();
    void shutdown();
  };

}