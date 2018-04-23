#pragma once
#include "hive_types.h"
#include "sc2_forward.h"
#include "hive_array2.h"

namespace hivemind {

  struct WeaponEffectSplashData {
    Real fraction_;
    Real radius_;
    WeaponEffectSplashData( Real fraction, Real radius ): fraction_( fraction ), radius_( radius ) {}
  };

  struct WeaponEffectData {
    vector<WeaponEffectData> sub_;
    bool hitsGround_;
    bool hitsAir_;
    bool hitsStructures_;
    bool hitsUnits_;
    bool isPersistent_;
    enum Kind {
      Unknown = 0,
      Melee,
      Splash,
      Ranged
    } kind_;
    Real damage_; //!< Base damage value.
    Real armorReduction_; //!< Amount of damage removed by 1 point of armor.
    vector<Real> attributeBonuses_; //!< Extra damage added to base value by enemy attributes.
    vector<WeaponEffectSplashData> splash_;
    bool dummy_;
    int persistentHitCount_;
    vector<Real> persistentPeriods_;
    WeaponEffectData(): kind_( Unknown ),
      hitsGround_( true ), hitsAir_( true ), hitsStructures_( true ), hitsUnits_( true ),
      damage_( 0.0f ), armorReduction_( 1.0f ), dummy_( true ), persistentHitCount_( 1 ),
      isPersistent_( false )
    {
      attributeBonuses_.resize( (size_t)Attribute::Invalid, 0.0f );
    }
  };

  struct UnitData;

  struct WeaponData {
  public:
    string name;
    Real arc;
    Real arcSlop;
    Real damagePoint; // Attack animation time point (in seconds) at which damage is applied to the target. Unit stands still while shooting.
    Real backSwing; // Attack animation duration (in seconds) after the shooting, that the unit stands reloading i.e. the initial value of weapon_cooldown. Can be canceled by issuing another order, but will not do that automatically (?)
    bool melee;
    Real minScanRange;
    Real period; // Cycle period between firings.
    Real randomDelayMax; // Minimum additional random delay between shots
    Real randomDelayMin; // Maximum additional random delay between shots
    Real range; // Range at which the attack animation can start.
    Real rangeSlop; // Extra distance the target can move beyond range, without the attack animation getting canceled.
    bool suicide; // If this weapon kills the casting unit.
    bool hitsGround;
    bool hitsAir;
    bool hitsStructures;
    bool hitsUnits;
    WeaponEffectData fx;
    WeaponData(): suicide( false ), hitsGround( false ), hitsAir( false ), hitsStructures( false ), hitsUnits( false ) {}
    // Returns damage & damgeTime_out (= time attack lasts.) Does not check for "can hit x" qualifiers.
    Real calculateDamageAgainst( const UnitData& against, Real& damageTime_out, Real& splashRange_out, int armor = 0 ) const;
    Real calculateAttributeBonuses( Attribute attrib ) const;
  };

  using WeaponDataMap = std::map<string, WeaponData>;

  using UnitType64 = size_t;
  using UnitTypeSet = set<UnitType64>;

  struct UnitData {
  public:
    enum Footprint: uint8_t {
      Footprint_Empty = 0,
      Footprint_Creep,
      Footprint_NearResource,
      Footprint_Reserved
    };
    enum ResourceType {
      Resource_None = 0,
      Resource_MineralsHarvestable, //!< Mineral patch
      Resource_GasBuildable, //!< Gas geyser
      Resource_GasHarvestable //!< Gas extractor
    };
  public:
    UnitType64 id;
    string name;
    Real acceleration;
    bool armored;
    bool biological;
    int cargoSize;
    Real supplyCost;
    Real supplyProvided;
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
    vector<string> weapons;
    vector<WeaponData*> resolvedWeapons_;
    ResourceType resource;
    UnitData(): supplyCost( 0.0f ), supplyProvided( 0.0f ) {}
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
    int unitCount;
    bool morphCancellable;
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

  struct BuildAbility {
    AbilityID ability;
    int unitCount;
    bool consumesSource;
    int mineralCost;
    int vespeneCost;
    int supplyCost;
  };

  using BuildAbilityMap = std::map<std::pair<UnitTypeID, UnitTypeID>, BuildAbility>;

  class Console;

  class TechTree {
  protected:
    TechTreeRelationshipContainer relationships_;
    UpgradeMap upgrades_;
    BuildAbilityMap buildAbilities_;
  public:

    void dump( Console* console ) const;
    void load( const string& filename );

    void findTechChain( UnitTypeID target, vector<UnitTypeID>& chain) const;

    UpgradeInfo findTechChain( UpgradeID target, vector<UnitTypeID>& chain ) const;
    inline const BuildAbility& getBuildAbility( UnitTypeID dest, UnitTypeID source ) const
    {
      return buildAbilities_.at( std::make_pair( dest, source ) );
    }

    UpgradeInfo getUpgradeInfo(UpgradeID upgradeType, UnitTypeID techBuildingType) const;
  };

  class Database {
  protected:
    static WeaponDataMap weaponData_;
    static UnitDataMap unitData_;
    static TechTree techTree_;

  public:
    static void load( const string& dataPath );
    inline static const UnitDataMap& units() { return unitData_; }
    inline static const UnitData& unit( UnitType64 id ) { return unitData_[id]; }
    inline static const WeaponDataMap& weapons() { return weaponData_; }

    // Get the last weapon of unit.
    inline static const WeaponData& weapon(UnitType64 id) {
      return weaponData_.at(unit(id).weapons.back());
    }

    inline static const TechTree& techTree() { return techTree_; }

    static void dumpWeapons();
  };

}