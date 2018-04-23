#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( base_debug, "Whether to show and print debug information on bases. 0 = none, 1 = basics, 2 = verbose", 1 );

  const size_t c_minersPerPatch = 2;
  const size_t c_collectorsPerGeyser = 3;

  Base::Base( BaseManager* owner, size_t index, BaseLocation* location, UnitRef depot ):
  manager_( owner ), index_( index ), location_( location )
  {
    assert( location );
    addDepot( depot );
  }

  size_t Base::WantedWorkers::total() const
  {
    return ( miners_ + collectors_ + overhead_ );
  }

  size_t Base::WantedQueens::total() const
  {
    return ( injectors_ + creepers_ + overhead_ );
  }

  Real Base::saturation() const
  {
    return ( (Real)workers_.size() / (Real)wantWorkers_.total() );
  }

  void Base::refresh()
  {
    wantWorkers_.collectors_ = ( location_->geysers_.size() * c_collectorsPerGeyser );
    wantWorkers_.overhead_ = 0;

    wantWorkers_.miners_ = 0;
    for(auto depot : depots_)
    {
      wantWorkers_.miners_ += depot->ideal_harvesters;
    }
  }

  void Base::draw( Bot* bot )
  {
    if ( g_CVar_base_debug.as_i() < 1 )
      return;

    uint8_t rgb[3];
    utils::hsl2rgb( (uint16_t)index_ + 42 * 120, 0xff, 220, rgb );
    sc2::Color color;
    color.r = rgb[0];
    color.g = rgb[1];
    color.b = rgb[2];

    char asd[128];
    sprintf_s( asd, 128, "Base %llu\nWorkers %llu\nQueens %llu\nLarvae %llu\nBuildings %llu",
      index_, workers_.size(), queens_.size(), larvae_.size(), buildings_.size() );
    bot->debug().drawText( asd, Vector3( location_->position_.x, location_ ->position_.y, bot->map().maxZ_ + 0.1f ), color );

    for ( auto building : depots_ )
      bot->debug().drawSphere( building->pos, building->radius, color );
    for ( auto building : buildings_ )
      bot->debug().drawSphere( building->pos, building->radius, color );
  }

  BaseLocation* Base::location() const
  {
    return location_;
  }

  void Base::_refreshWorkers()
  {
  }

  void Base::_refreshQueens()
  {
  }

  UnitRef Base::releaseWorker()
  {
    if(workers_.empty())
      return nullptr;

    auto worker = *workers_.begin();
    workers_.erase(workers_.begin());

    return worker;
  }

  void Base::releaseQueen(UnitRef queen)
  {
    queens_.erase(queen);
  }

  UnitSet Base::releaseWorkers( int count )
  {
    UnitSet set;
    for ( int i = 0; i < count; i++ )
    {
      set.insert(releaseWorker());
    }
    return set;
  }

  const Base::WantedWorkers & Base::wantWorkers() const
  {
    return wantWorkers_;
  }

  const Base::WantedQueens & Base::wantQueens() const
  {
    return wantQueens_;
  }

  const UnitSet & Base::workers() const
  {
    return workers_;
  }

  const UnitSet & Base::queens() const
  {
    return queens_;
  }

  const UnitSet & Base::buildings() const
  {
    return buildings_;
  }

  vector<Base::Refinery> & Base::refineries()
  {
    return refineries_;
  }

  const UnitSet & Base::larvae() const
  {
    return larvae_;
  }

  const UnitSet& Base::depots() const
  {
    return depots_;
  }

  UnitRef Base::queen()
  {
    if ( queens_.empty() )
      return nullptr;
    // todo better
    auto it = queens_.cbegin();
    std::advance( it, utils::randomBetween( 0, (int)queens_.size() - 1 ) );
    return ( *it );
  }

  bool Base::hasWorker( UnitRef worker ) const
  {
    return ( workers_.find( worker ) != workers_.end() );
  }

  bool Base::hasQueen( UnitRef queen ) const
  {
    return ( queens_.find( queen ) != queens_.end() );
  }

  void Base::addWorker( UnitRef worker, bool refresh )
  {
    workers_.insert( worker );
    if ( refresh )
      _refreshWorkers();
  }

  void Base::addWorkers( const UnitSet & workers )
  {
    for ( auto worker : workers )
      addWorker( worker );
  }

  void Base::addQueen( UnitRef queen, bool refresh )
  {
    queens_.insert( queen );
    if ( refresh )
      _refreshQueens();
  }

  void Base::addQueens( const UnitSet & queens )
  {
    for ( auto queen : queens )
      addQueen( queen );
  }

  void Base::addLarva( UnitRef larva )
  {
    larvae_.insert( larva );
  }

  void Base::removeLarva( UnitRef unit )
  {
    larvae_.erase( unit );
  }

  void Base::remove( UnitRef unit )
  {
    // Note: This gets called a lot for units that we don't own in the first place
    workers_.erase( unit );
    larvae_.erase( unit );
    queens_.erase( unit );
    buildings_.erase( unit );

    for(auto it = refineries_.begin(); it != refineries_.end(); ++it)
    {
      if(it->refinery_ == unit)
      {
        for(auto worker : it->workers_)
        {
          addWorker(worker);
        }
        refineries_.erase(it);
        break;
      }
    }

    for(auto& x : refineries_)
    {
      x.workers_.erase(unit);
    }
  }

  void Base::addDepot( UnitRef depot )
  {
    if ( g_CVar_base_debug.as_i() > 1 )
      manager_->bot_->console().printf( "Base %llu: Adding depot %x", index_, hivemind::id( depot ) );

    depots_.insert( depot );
  }

  void Base::addBuilding( UnitRef building )
  {
    if ( g_CVar_base_debug.as_i() > 1 )
      manager_->bot_->console().printf( "Base %llu: Adding building %x", index_, hivemind::id( building ) );

    buildings_.insert( building );

    if ( utils::isRefinery( building ) )
    {
      refineries_.push_back( { building, {} } );
    }
  }

  void Base::update( Bot& bot )
  {
    // For whatever reason, larvae/eggs don't get "destroyed" events when they morph to other units.
    // So we have to check whether they still exist and clean up ourselves.
    for ( UnitSet::iterator it = larvae_.begin(); it != larvae_.end(); )
    {
      if ( !( *it )->is_alive )
      {
        if ( g_CVar_base_debug.as_i() > 1 )
          manager_->bot_->console().printf( "Base %llu: Removing larva %x", index_, hivemind::id( *it ) );

        it = larvae_.erase( it );
      }
      else
        it++;
    }
  }

}