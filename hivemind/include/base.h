#pragma once
#include "sc2_forward.h"
#include "baselocation.h"
#include "workers.h"

namespace hivemind {

  enum BuildingPlacement {
    BuildPlacement_Front, //!< Weighted toward front of the base
    BuildPlacement_Back, //!< Weighted toward back of the base
    BuildPlacement_MineralLine, //!< In the mineral line
    BuildPlacement_Choke, //!< At the choke/ramp
    BuildPlacement_Hidden //!< As stealthy as possible
  };

  class Base {
  public:
    struct WantedWorkers {
      size_t miners_; //!< Ideal amount of miners
      size_t collectors_; //!< Ideal amount of gas collectors
      size_t overhead_; //!< Ideal amount of overhead workers in preparation for usage
      size_t total() const; //!< Total ideal amount of workers
    };
    struct WantedQueens {
      size_t injectors_; //!< Ideal amount of injectors, basically one
      size_t creepers_; //!< Ideal amount of creep spreaders
      size_t overhead_; //!< Ideal amount of overhead queens
      size_t total() const; //!< Total ideal amount of queens
    };
  private:
    size_t index_;
    BaseLocation* location_; //!< Base location
    TagSet workers_; //!< Workers in this base
    TagSet queens_; //!< Queens assigned to this base
    WantedWorkers wantWorkers_; //!< Wanted worker counts
    WantedQueens wantQueens_; //!< Wanted queen counts
    UnitMap buildings_; //!< Buildings in this base
    TagSet larvae_;
    TagSet depots_;
    void _refreshWorkers();
    void _refreshQueens();
  public:
    Real mineralWeight_; //!< Importance of producing minerals, 0..1
    Real gasWeight_; //!< Importance of producing gas, 0..1
    Real buildingWeight_; //!< Importance of having tech, 0..1
    Real importance_; //!< Importance of this base, 0..1
  public:
    void refresh();
    inline const size_t id() const { return index_; }
    BaseLocation* location() const; //!< Base location
    Real saturation() const; //!< Worker saturation between 0..1
    TagSet releaseWorkers( int count ); //!< Release a set of workers for other use
    TagSet releaseQueens( int count ); //!< Release a set of queens for other use
    const WantedWorkers& wantWorkers() const; //!< Wanted worker counts
    const WantedQueens& wantQueens() const; //!< Wanted queen counts
    const TagSet& workers() const; //!< Return our set of workers
    const TagSet& queens() const; //!< Return our set of queens
    const UnitMap& buildings() const; //!< Return our buildings
    bool hasWorker( Tag worker ) const; //!< Is a given worker mine?
    bool hasQueen( Tag queen ) const; //!< Is a given queen mine?
    void addWorker( Tag worker, bool refresh = true ); //!< Assign a worker to this base
    void addWorkers( const TagSet& workers ); //!< Assign workers to this base
    void addQueen( Tag queen, bool refresh = true ); //!< Assign a queen to this base
    void addQueens( const TagSet& queens ); //!< Assign queens to this base
    void addLarva( Tag larva );
    void onDestroyed( Tag unit );
    void addDepot( const Unit& depot );
    void addBuilding( const Unit& building );
    Point2D findBuildingPlacement( UnitTypeID structure, BuildingPlacement type );
    Base( size_t index, BaseLocation* location, const Unit& depot );
  };

  using BaseVector = vector<Base>;

}