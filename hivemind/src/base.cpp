#include "stdafx.h"
#include "base.h"
#include "utilities.h"

namespace hivemind {

  Base::Base( BaseLocation* location, const TagSet& workers, const UnitMap& buildings ):
  location_( location ), workers_( workers ), buildings_( buildings )
  {
    assert( location );
    printf( "BASE::BASE: loc %d, workers %d, buildings %d\r\n", location_->baseID_, workers_.size(), buildings_.size() );
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
  {}

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

  Point2D Base::findBuildingPlacement( UnitTypeID structure, BuildingPlacement type )
  {
    return Point2D();
  }

}