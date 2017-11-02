#pragma once
#include "hive_types.h"
#include "sc2_forward.h"

namespace hivemind {

  using UnitType64 = size_t;
  using UnitTypeSet = set<UnitType64>;

  struct UnitData {
    UnitType64 id;
    string name;
    Real acceleration;
    bool armored;
    bool biological;
    int cargoSize;
    int food;
    int lifeArmor;
    int lifeMax;
    Real lifeRegenRate;
    int lifeStart;
    bool light;
    bool massive;
    bool mechanical;
    int mineralCost;
    bool flying;
    bool psionic;
    Race race;
    Real radius;
    int scoreKill;
    int scoreMake;
    int shieldRegenDelay;
    Real shieldRegenRate;
    int shieldsMax;
    int shieldsStart;
    Real sight;
    Real speed;
    Real speedMultiplierCreep;
    bool structure;
    Real turningRate;
    int vespeneCost;
    Array2<bool> footprint; //!< Note: footprint has x & y flipped, otherwise there's a heap corruption on data load. (wtf?)
    Point2DI footprintOffset;
    inline operator UnitTypeID() const
    {
      UnitTypeID ret( (uint32_t)id );
      return ret;
    }
  };

  using UnitDataMap = std::map<UnitType64, UnitData>;

  struct TechTreeUnitRequirement {
    UnitTypeIDSet units; //!< One of (aliases)
    bool not; //!< Is this a negative requirement (may only have one mothership)
    bool attached; //!< Must be attached, such as a tech lab
  };

  using TechTreeUnitRequirements = vector<TechTreeUnitRequirement>;

  struct TechTreeUpgradeRequirement {
    UpgradeID upgrade;
  };

  using TechTreeUpgradeRequirements = vector<TechTreeUpgradeRequirement>;

  struct TechTreeRelationship {
    string name; //!< Name of the described unit type.
    UnitTypeID source; //!< Which unit can build the described unit.
    AbilityID ability; //!< Using which ability.
    Real time; //!< In how long a time.
    bool isMorph; //!< Is this a morph, i.e. consumes source unit.
    TechTreeUnitRequirements unitRequirements; //!< Additional unit requirements TODO parse
    TechTreeUpgradeRequirements upgradeRequirements; //!< Additional upgrade requirements TODO parse
  };

  // Maps a unit type id to the description on how to build the unit type.
  using TechTreeRelationshipContainer = std::unordered_multimap<uint32_t, TechTreeRelationship>;

  class TechTree {
  protected:
    TechTreeRelationshipContainer relationships_;

  public:
    void load( const string& filename );
    vector<UnitTypeID> findTechChain( UnitTypeID target ) const;
    void findTechChain( UnitTypeID target, vector<UnitTypeID>& chain) const;
  };

  class Database {
  protected:
    static UnitDataMap unitData_;
    static TechTree techTree_;
  public:
    static void load( const string& dataPath );
    inline static const UnitDataMap& units() { return unitData_; }
    inline static const UnitData& unit( UnitType64 id ) { return unitData_[id]; }
    inline static const TechTree& techTree() { return techTree_; }
  };

}