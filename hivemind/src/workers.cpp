#include "stdafx.h"
#include "workers.h"
#include "bot.h"
#include "utilities.h"

namespace hivemind {

  void WorkerManager::_created( UnitRef unit )
  {
    if ( exists( unit ) )
      return;

    add( unit );
  }

  void WorkerManager::_destroyed( UnitRef unit )
  {
    remove( unit );
  }

  void WorkerManager::_idle( UnitRef unit )
  {
    if ( ignored( unit ) )
      return;
  }

  void WorkerManager::_onIdle( UnitRef worker )
  {
    idle_.insert( worker );
  }

  void WorkerManager::_onActivate( UnitRef worker )
  {
    idle_.erase( worker );
  }

  void WorkerManager::_refreshWorkers()
  {
    auto workers = bot_->observation().GetUnits( Unit::Alliance::Self, []( const Unit& unit ) -> bool {
      return utils::isWorker( unit );
    } );

    for ( auto worker : workers )
      if ( !exists( worker ) )
        _created( worker );

    UnitSet removals;
    for ( auto worker : workers_ )
    {
      if ( !worker || !utils::isMine( worker ) || !worker->is_alive || !utils::isWorker( worker ) )
        removals.insert( worker );
    }

    for ( auto worker : removals )
      remove( worker );
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

  const UnitSet & WorkerManager::all() const
  {
    return workers_;
  }

  void WorkerManager::onMessage( const Message & msg )
  {
    if ( msg.code == M_Global_UnitCreated || msg.code == M_Global_UnitIdle || msg.code == M_Global_UnitDestroyed )
    {
      if ( !utils::isWorker( msg.unit() ) || !utils::isMine( msg.unit() ) )
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

  bool WorkerManager::add( UnitRef worker )
  {
    if ( ignored( worker ) )
      return false;

    if ( exists( worker ) )
      return true;

    if ( !worker || !utils::isWorker( worker ) || !utils::isMine( worker ) || !worker->is_alive )
      return false;

    workers_.insert( worker );

    bot_->console().printf( "GLOBAL: ADD WORKER %x", worker );
    bot_->messaging().sendGlobal( M_Global_AddWorker, worker );

    return true;
  }

  bool WorkerManager::addBack( UnitRef worker )
  {
    ignored_.erase( worker );
    return add( worker );
  }

  const bool WorkerManager::exists( UnitRef worker ) const
  {
    return ( workers_.find( worker ) != workers_.end() );
  }

  const bool WorkerManager::ignored( UnitRef worker ) const
  {
    return ( ignored_.find( worker ) != ignored_.end() );
  }

  const bool WorkerManager::idle( UnitRef worker ) const
  {
    return ( idle_.find( worker ) != idle_.end() );
  }

  void WorkerManager::update( GameTime time )
  {
    UnitSet removals;

    _refreshWorkers();

    for ( auto worker : workers_ )
    {
      if ( worker && worker->is_alive )
      {
        bool iddle = ( worker->orders.empty() );
        if ( iddle && !idle( worker ) )
          _onIdle( worker );
        else if ( !iddle && idle( worker ) )
          _onActivate( worker );
      }
      else
        removals.insert( worker );
    }
    for ( auto worker : removals )
      remove( worker );
  }

  void WorkerManager::remove( UnitRef worker )
  {
    workers_.erase( worker );
    ignored_.erase( worker );
    idle_.erase( worker );

    bot_->console().printf( "GLOBAL: REMOVE WORKER %x", worker );
    bot_->messaging().sendGlobal( M_Global_RemoveWorker, worker );
  }

  UnitRef WorkerManager::release()
  {
    if ( workers_.empty() )
      return nullptr;

    auto it = workers_.cbegin();
    std::advance( it, utils::randomBetween( 0, (int)workers_.size() - 1 ) );

    auto worker = ( *it );

    bot_->console().printf( "GLOBAL: RELEASE WORKER %x", worker );
    bot_->messaging().sendGlobal( M_Global_RemoveWorker, worker );

    workers_.erase( worker );
    idle_.erase( worker );
    ignored_.insert( worker );

    return worker;
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

    sprintf_s( text, 32, "Total %zd", workers_.size() );
    bot_->debug().DebugTextOut( text, pos, sc2::Colors::Blue );
    pos.y += increment;

    sprintf_s( text, 32, "Active %zd", workers_.size() - idle_.size() );
    bot_->debug().DebugTextOut( text, pos, sc2::Colors::Green );
    pos.y += increment;

    sprintf_s( text, 32, "Idle %zd", idle_.size() );
    bot_->debug().DebugTextOut( text, pos, sc2::Colors::Yellow );
    pos.y += increment;

    sprintf_s( text, 32, "Released %zd", ignored_.size() );
    bot_->debug().DebugTextOut( text, pos, sc2::Colors::Purple );
  }

}