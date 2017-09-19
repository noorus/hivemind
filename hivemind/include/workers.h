#pragma once
#include "sc2_forward.h"
#include "messaging.h"

namespace hivemind {

  class WorkerManager: public Subsystem, private Listener {
  private:
    TagSet ignored_; //! Workers that have been released for other use and not managed by us
    TagSet workers_;
    TagSet idle_;
    void _created( const Unit* unit );
    void _destroyed( const Unit* unit );
    void _idle( const Unit* unit );
    void _onIdle( Tag worker );
    void _onActivate( Tag worker );
    void _refreshWorkers();
  private:
    void onMessage( const Message& msg ) final;
  public:
    WorkerManager( Bot* bot );
    virtual void gameBegin() override;
    virtual void gameEnd() override;
    const TagSet& all() const;
    bool add( Tag worker );
    const bool exists( Tag worker ) const;
    const bool ignored( Tag worker ) const;
    const bool idle( Tag worker ) const;
    void remove( Tag worker );
    Tag release(); //! Release a worker for other use
    virtual void draw() override;
    void update( GameTime time );
    void initialise();
    void shutdown();
  };

}