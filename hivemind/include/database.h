#pragma once
#include "hive_types.h"
#include "sc2_forward.h"

namespace hivemind {

  using UnitType64 = size_t;
  using UnitTypeSet = set<UnitType64>;

  struct UnitData {
    enum Footprint: uint8_t {
      Footprint_Empty = 0,
      Footprint_Creep,
      Footprint_NearResource,
      Footprint_Reserved
    };
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
    Array2<Footprint> footprint; //!< Note: footprint has x & y flipped, otherwise there's a heap corruption on data load. (wtf?)
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

  using TechTreeUpgradeRequirements = vector<UpgradeID>;

  struct TechTreeRelationship {
    string name; //!< Name of the described unit type.
    UnitTypeID source; //!< Which unit can build the described unit.
    AbilityID ability; //!< Using which ability.
    Real time; //!< In how long a time.
    bool isMorph; //!< Is this a morph, i.e. consumes source unit.
    TechTreeUnitRequirements unitRequirements; //!< Additional unit requirements.
  };

  struct UpgradeInfo
  {
    string name;
    UpgradeID target;
    UnitTypeID techBuilding;
    AbilityID ability;
    Real time;

    TechTreeUnitRequirements unitRequirements; //!< Additional unit requirements.
    TechTreeUpgradeRequirements upgradeRequirements; //!< Additional upgrade requirements.
  };

  // Maps a unit type id to the description on how to build the unit type.
  using TechTreeRelationshipContainer = std::unordered_multimap<uint32_t, TechTreeRelationship>;
  // Maps a upgrade type id to the description on how to research the upgrade type.
  using UpgradeMap = std::unordered_multimap<uint32_t, UpgradeInfo>;

  class TechTree {
  protected:
    TechTreeRelationshipContainer relationships_;
    UpgradeMap upgrades_;

  public:
    void load( const string& filename );

    void findTechChain( UnitTypeID target, vector<UnitTypeID>& chain) const;

    UpgradeInfo findTechChain( UpgradeID target, vector<UnitTypeID>& chain ) const;
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