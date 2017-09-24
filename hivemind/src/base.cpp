#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"

namespace hivemind {

  const size_t c_minersPerPatch = 2;
  const size_t c_collectorsPerGeyser = 3;

  Base::Base( BaseManager* owner, size_t index, BaseLocation* location, const Unit& depot ):
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
    wantWorkers_.miners_ = ( location_->minerals_.size() * c_minersPerPatch );
    wantWorkers_.collectors_ = ( location_->geysers_.size() * c_collectorsPerGeyser );
    wantWorkers_.overhead_ = 0;
  }

  void Base::draw( Bot* bot )
  {
    uint8_t rgb[3];
    utils::hsl2rgb( (uint16_t)index_ + 42 * 120, 0xff, 220, rgb );
    sc2::Color color;
    color.r = rgb[0];
    color.g = rgb[1];
    color.b = rgb[2];

    char asd[128];
    sprintf_s( asd, 128, "Base %llu\nWorkers %llu\nQueens %llu\nLarvae %llu\nBuildings %llu",
      index_, workers_.size(), queens_.size(), larvae_.size(), buildings_.size() );
    bot->debug().DebugTextOut( asd, Point3D( location_->position_.x, location_ ->position_.y, bot->map().maxZ_ + 0.1f ), color );

    for ( auto& it : depots_ )
    {
      auto building = bot->observation().GetUnit( it );
      if ( building )
        bot->debug().DebugSphereOut( building->pos, building->radius, color );
    }
    for ( auto& it : buildings_ )
    {
      auto building = bot->observation().GetUnit( it.first );
      if ( building )
        bot->debug().DebugSphereOut( building->pos, building->radius, color );
    }
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

  TagSet Base::releaseWorkers( int count )
  {
    TagSet set;
    for ( int i = 0; i < count; i++ )
    {
      /*for ( auto worker : workers_ ) {
      if (  )
      }*/
    }
    return set;
  }

  TagSet Base::releaseQueens( int count )
  {
    TagSet set;
    for ( int i = 0; i < count; i++ )
    {
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

  const TagSet & Base::workers() const
  {
    return workers_;
  }

  const TagSet & Base::queens() const
  {
    return queens_;
  }

  const UnitMap & Base::buildings() const
  {
    return buildings_;
  }

  const TagSet & Base::larvae() const
  {
    return larvae_;
  }

  const TagSet& Base::depots() const
  {
    return depots_;
  }

  bool Base::hasWorker( Tag worker ) const
  {
    return ( workers_.find( worker ) != workers_.end() );
  }

  bool Base::hasQueen( Tag queen ) const
  {
    return ( queens_.find( queen ) != queens_.end() );
  }

  void Base::addWorker( Tag worker, bool refresh )
  {
    workers_.insert( worker );
    if ( refresh )
      _refreshWorkers();
  }

  void Base::addWorkers( const TagSet & workers )
  {
    for ( auto worker : workers )
      addWorker( worker );
  }

  void Base::addQueen( Tag queen, bool refresh )
  {
    queens_.insert( queen );
    if ( refresh )
      _refreshQueens();
  }

  void Base::addQueens( const TagSet & queens )
  {
    for ( auto queen : queens )
      addQueen( queen );
  }

  void Base::addLarva( Tag larva )
  {
    larvae_.insert( larva );
  }

  void Base::remove( Tag unit )
  {
    // Note: This gets called a lot for units that we don't own in the first place
    workers_.erase( unit );
    larvae_.erase( unit );
    queens_.erase( unit );
    buildings_.erase( unit );
  }

  void Base::addDepot( const Unit & depot )
  {
    depots_.insert( depot.tag );
  }

  void Base::addBuilding( const Unit & building )
  {
    buildings_[building.tag] = building.unit_type;
  }

  void Base::update( Bot & bot )
  {
    // For whatever reason, larvae/eggs don't get "destroyed" events when they morph to other units.
    // So we have to check whether they still exist and clean up ourselves.
    for ( TagSet::iterator it = larvae_.begin(); it != larvae_.end(); )
    {
      auto unit = bot.observation().GetUnit( (*it) );
      if ( !unit )
      {
        manager_->bot_->console().printf( "Base %llu: Removing larva %llu", index_, ( *it ) );
        it = larvae_.erase( it );
      }
      else
        it++;
    }
  }

  Point2D Base::findBuildingPlacement( UnitTypeID structure, BuildingPlacement type )
  {
    return Point2D();
  }

}