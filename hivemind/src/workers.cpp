#include "stdafx.h"
#include "workers.h"
#include "bot.h"
#include "utilities.h"

namespace hivemind {

  void WorkerManager::_created( const Unit* unit )
  {
    if ( exists( unit->tag ) )
      return;

    add( unit->tag );
  }

  void WorkerManager::_destroyed( const Unit* unit )
  {
    remove( unit->tag );
  }

  void WorkerManager::_idle( const Unit* unit )
  {
    if ( ignored( unit->tag ) )
      return;
  }

  void WorkerManager::_onIdle( Tag worker )
  {
    idle_.insert( worker );
  }

  void WorkerManager::_onActivate( Tag worker )
  {
    idle_.erase( worker );
  }

  void WorkerManager::_refreshWorkers()
  {
    auto workers = bot_->observation().GetUnits( Unit::Alliance::Self, []( const Unit& unit ) -> bool {
      return utils::isWorker( unit );
    } );

    for ( auto& worker : workers )
      if ( !exists( worker.tag ) )
        _created( &worker );
  }

  WorkerManager::WorkerManager( Bot* bot ): Subsystem( bot )
  {
    //
  }

  void WorkerManager::gameBegin()
  {
    initialise();
  }

  void WorkerManager::gameEnd()
  {
    shutdown();
  }

  const TagSet & WorkerManager::all() const
  {
    return workers_;
  }

  void WorkerManager::onMessage( const Message & msg )
  {
    if ( msg.code == M_Global_UnitCreated || msg.code == M_Global_UnitIdle || msg.code == M_Global_UnitDestroyed )
    {
      if ( !utils::isWorker( *msg.unit() ) || !utils::isMine( *msg.unit() ) )
        return;
      if ( msg.code == M_Global_UnitCreated )
        _created( msg.unit() );
      else if ( msg.code == M_Global_UnitDestroyed )
        _destroyed( msg.unit() );
      else if ( msg.code == M_Global_UnitIdle )
        _idle( msg.unit() );
    }
  }

  void WorkerManager::initialise()
  {
    shutdown();

    bot_->messaging().listen( Listen_Global, this );

    _refreshWorkers();
  }

  bool WorkerManager::add( Tag worker )
  {
    if ( exists( worker ) )
      return true;

    auto unit = bot_->observation().GetUnit( worker );
    if ( !unit || !utils::isWorker( *unit ) || !utils::isMine( *unit ) )
      return false;

    workers_.insert( worker );
    return true;
  }

  const bool WorkerManager::exists( Tag worker ) const
  {
    return ( workers_.find( worker ) != workers_.end() );
  }

  const bool WorkerManager::ignored( Tag worker ) const
  {
    return ( ignored_.find( worker ) != ignored_.end() );
  }

  const bool WorkerManager::idle( Tag worker ) const
  {
    return ( idle_.find( worker ) != idle_.end() );
  }

  void WorkerManager::update( GameTime time )
  {
    TagSet removals;

    _refreshWorkers();

    for ( auto& worker : workers_ )
    {
      auto unit = bot_->observation().GetUnit( worker );
      if ( unit ) {
        bool iddle = ( unit->orders.empty() );
        if ( iddle && !idle( worker ) )
          _onIdle( worker );
        else if ( !iddle && idle( worker ) )
          _onActivate( worker );
      }
      else
        removals.insert( worker );
    }
    for ( auto& worker : removals )
      remove( worker );
  }

  void WorkerManager::remove( Tag worker )
  {
    workers_.erase( worker );
    ignored_.erase( worker );
    idle_.erase( worker );
  }

  Tag WorkerManager::release()
  {
    if ( workers_.empty() )
      return 0;

    auto it = workers_.cbegin();
    std::advance( it, utils::randomBetween( 0, (int)workers_.size() - 1 ) );

    auto tag = ( *it );
    workers_.erase( it );
    idle_.erase( tag );
    ignored_.insert( tag );

    return tag;
  }

  void WorkerManager::shutdown()
  {
    workers_.clear();
    ignored_.clear();
    idle_.clear();
  }

  void WorkerManager::draw()
  {
    Point2D pos( 0.03f, 0.225f );
    Real increment = 0.01f;
    char text[32];

    bot_->debug().DebugTextOut( "WORKERS", pos, sc2::Colors::White );
    pos.y += increment;

    sprintf_s( text, 32, "Active %zd", workers_.size() - idle_.size() - ignored_.size() );
    bot_->debug().DebugTextOut( text, pos, sc2::Colors::Green );
    pos.y += increment;

    sprintf_s( text, 32, "Idle %zd", idle_.size() );
    bot_->debug().DebugTextOut( text, pos, sc2::Colors::Yellow );
    pos.y += increment;

    sprintf_s( text, 32, "Released %zd", ignored_.size() );
    bot_->debug().DebugTextOut( text, pos, sc2::Colors::Purple );
  }

}