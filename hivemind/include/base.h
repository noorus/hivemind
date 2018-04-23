#pragma once
#include "sc2_forward.h"
#include "baselocation.h"
#include "workers.h"
#include "console.h"

namespace hivemind {

  HIVE_EXTERN_CONVAR( base_debug );

  class BaseManager;

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
    struct Refinery
    {
      UnitRef refinery_;
      UnitSet workers_;
    };
  private:
    BaseManager* manager_;
    size_t index_;
    BaseLocation* location_; //!< Base location
    UnitSet workers_; //!< Workers in this base
    UnitSet queens_; //!< Queens assigned to this base
    WantedWorkers wantWorkers_; //!< Wanted worker counts
    WantedQueens wantQueens_; //!< Wanted queen counts
    UnitSet buildings_; //!< Buildings in this base
    vector<Refinery> refineries_; //!< Refineries and their workers in this base.
    UnitSet larvae_;
    UnitSet depots_;
    void _refreshWorkers();
    void _refreshQueens();
  public:
    Real mineralWeight_; //!< Importance of producing minerals, 0..1
    Real gasWeight_; //!< Importance of producing gas, 0..1
    Real buildingWeight_; //!< Importance of having tech, 0..1
    Real importance_; //!< Importance of this base, 0..1
  public:
    void refresh();
    void draw( Bot* bot );
    inline BaseManager* manager() { return manager_; }
    inline const size_t id() const { return index_; }
    BaseLocation* location() const; //!< Base location
    Real saturation() const; //!< Worker saturation between 0..1
    UnitRef releaseWorker(); //!< Release a worker for other use
    UnitSet releaseWorkers( int count ); //!< Release a set of workers for other use
    void releaseQueen(UnitRef queen); //!< Release a queen for other use
    const WantedWorkers& wantWorkers() const; //!< Wanted worker counts
    const WantedQueens& wantQueens() const; //!< Wanted queen counts
    const UnitSet& workers() const; //!< Return our set of workers
    const UnitSet& queens() const; //!< Return our set of queens
    const UnitSet& buildings() const; //!< Return our buildings
    vector<Refinery>& refineries(); //!< Return our extractors
    const UnitSet& larvae() const; //!< Return our larvae
    const UnitSet& depots() const; //!< Return our main buildings
    UnitRef queen();
    bool hasWorker( UnitRef worker ) const; //!< Is a given worker mine?
    bool hasQueen( UnitRef queen ) const; //!< Is a given queen mine?
    void addWorker( UnitRef worker, bool refresh = true ); //!< Assign a worker to this base
    void addWorkers( const UnitSet& workers ); //!< Assign workers to this base
    void addQueen( UnitRef queen, bool refresh = true ); //!< Assign a queen to this base
    void addQueens( const UnitSet& queens ); //!< Assign queens to this base
    void addLarva( UnitRef larva );
    void removeLarva( UnitRef larva );
    void remove( UnitRef unit );
    void addDepot( UnitRef depot );
    void addBuilding( UnitRef building );
    void update( Bot& bot );
    Base( BaseManager* owner, size_t index, BaseLocation* location, UnitRef depot );
  };

  using BaseVector = vector<Base>;

}