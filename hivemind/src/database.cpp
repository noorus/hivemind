#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"

namespace hivemind {

  WeaponDataMap Database::weaponData_;
  UnitDataMap Database::unitData_;
  TechTree Database::techTree_;

  static UnitTypeID getUnitTypeByName(const string& name)
  {
    for(const auto& x : Database::units())
    {
      if(x.second.name == name)
      {
        return (uint32_t)x.first;
      }
    }
    assert(false && "no unit type found by name");
    return sc2::UNIT_TYPEID::INVALID;
  }

  static UnitTypeID normalizeUnitType(UnitTypeID unitType)
  {
    const auto& name = Database::unit(unitType).name;
    for(auto suffix : { "Burrowed", "Lowered", "Flying", "Sieged" })
    {
      if(name.find(suffix) != string::npos)
      {
        return getUnitTypeByName(name.substr(0, name.size() - strlen(suffix)));
      }
    }
    return unitType;
  }

  static Json::Value readJsonFile(const string& filename)
  {
    std::ifstream infile(filename);

    if(!infile)
    {
      HIVE_EXCEPT("Failed to open '" + filename + "'");
    }

    Json::Value root;
    infile >> root;
    return root;
  }

  WeaponEffectData parseEffectData( const Json::Value& effect, WeaponData& weapon )
  {
    WeaponEffectData fx;
    if ( !effect.isMember( "type" ) )
      return fx;
    auto type = effect["type"].asString();
    if ( boost::iequals( type, "set" ) || boost::iequals( type, "persistent" ) )
    {
      fx.dummy_ = false;
      for ( auto& sub : effect["setEffects"] )
      {
        auto tmp = parseEffectData( sub, weapon );
        if ( !tmp.dummy_ ) // Don't bother adding effects we were unable to identify
          fx.sub_.push_back( tmp );
      }
      if ( boost::iequals( type, "persistent" ) )
      {
        fx.isPersistent_ = true;
        for ( auto& period : effect["persistentPeriods"] )
          fx.persistentPeriods_.push_back( period.asFloat() );
        if ( fx.persistentPeriods_.empty() )
          fx.persistentPeriods_.push_back( 0.0f );
        if ( effect.isMember( "persistentCount" ) )
          fx.persistentHitCount_ = effect["persistentCount"].asInt();
        if ( fx.persistentHitCount_ < 1 )
          fx.persistentHitCount_ = fx.persistentPeriods_.size();
      }
    }
    else if ( boost::iequals( type, "suicide" ) )
    {
      weapon.suicide = true;
    }
    else if ( boost::iequals( type, "missile" ) )
    {
      fx.dummy_ = false;
      if ( effect.isMember( "impact" ) )
      {
        auto tmp = parseEffectData( effect["impact"], weapon );
        if ( !tmp.dummy_ ) // Don't bother adding effects we were unable to identify
          fx.sub_.push_back( tmp );
      }
    }
    else if ( boost::iequals( type, "damage" ) )
    {
      fx.dummy_ = false;
      fx.damage_ = effect["dmgAmount"].asFloat();
      fx.armorReduction_ = effect["dmgArmorReduction"].asFloat();
      for ( auto& splash : effect["dmgSplash"] )
        fx.splash_.emplace_back( splash["fraction"].asFloat(), splash["radius"].asFloat() );
      if ( effect.isMember( "dmgAttributeBonuses" ) )
      {
        const auto& bonuses = effect["dmgAttributeBonuses"];
        for ( auto it = bonuses.begin(); it != bonuses.end(); it++ )
        {
          if ( boost::iequals( it.key().asString(), "Light" ) )
            fx.attributeBonuses_[(size_t)Attribute::Light] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Armored" ) )
            fx.attributeBonuses_[(size_t)Attribute::Armored] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Biological" ) )
            fx.attributeBonuses_[(size_t)Attribute::Biological] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Mechanical" ) )
            fx.attributeBonuses_[(size_t)Attribute::Mechanical] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Robotic" ) )
            fx.attributeBonuses_[(size_t)Attribute::Robotic] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Psionic" ) )
            fx.attributeBonuses_[(size_t)Attribute::Psionic] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Massive" ) )
            fx.attributeBonuses_[(size_t)Attribute::Massive] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Structure" ) )
            fx.attributeBonuses_[(size_t)Attribute::Structure] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Hover" ) )
            fx.attributeBonuses_[(size_t)Attribute::Hover] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Heroic" ) )
            fx.attributeBonuses_[(size_t)Attribute::Heroic] = ( *it ).asFloat();
          else if ( boost::iequals( it.key().asString(), "Summoned" ) )
            fx.attributeBonuses_[(size_t)Attribute::Summoned] = ( *it ).asFloat();
        }
      }
      if ( effect.isMember( "searchRequires" ) )
        for ( auto& sub : effect["searchRequires"] )
          if ( boost::iequals( sub.asString(), "ground" ) )
          {
            fx.hitsGround_ = true;
            fx.hitsAir_ = false;
          }
          else if ( boost::iequals( sub.asString(), "air" ) )
          {
            fx.hitsGround_ = false;
            fx.hitsAir_ = true;
          }
          else if ( boost::iequals( sub.asString(), "structure" ) )
          {
            fx.hitsStructures_ = true;
            fx.hitsUnits_ = false;
          }
      if ( effect.isMember( "searchExcludes" ) )
        for ( auto& sub : effect["searchExcludes"] )
          if ( boost::iequals( sub.asString(), "ground" ) )
            fx.hitsGround_ = false;
          else if ( boost::iequals( sub.asString(), "air" ) )
            fx.hitsAir_ = false;
          else if ( boost::iequals( sub.asString(), "structure" ) )
            fx.hitsStructures_ = false;
    }
    return fx;
  }

  void loadWeaponData(const string& filename, WeaponDataMap& weaponData)
  {
    weaponData.clear();

    const Json::Value root = readJsonFile(filename);

    for(auto& value : root)
    {
      auto name = value["name"].asCString();
      auto& weapon = weaponData[name];
      weapon.name = name;

      weapon.arc = value["arc"].asFloat();
      weapon.arcSlop = value["arcSlop"].asFloat();
      weapon.backSwing = value["backSwing"].asFloat();
      weapon.damagePoint = value["damagePoint"].asFloat();
      weapon.melee = value["melee"].asBool();
      weapon.minScanRange = value["minScanRange"].asFloat();
      weapon.period = value["period"].asFloat();
      weapon.randomDelayMax = value["randomDelayMax"].asFloat();
      weapon.randomDelayMin = value["randomDelayMin"].asFloat();
      weapon.range = value["range"].asFloat();
      weapon.rangeSlop = value["rangeSlop"].asFloat();

      if ( value.isMember( "filterRequires" ) )
        for ( auto& sub : value["filterRequires"] )
          if ( boost::iequals( sub.asString(), "ground" ) )
          {
            weapon.hitsGround = true;
            weapon.hitsAir = false;
          }
          else if ( boost::iequals( sub.asString(), "air" ) )
          {
            weapon.hitsGround = false;
            weapon.hitsAir = true;
          }
          else if ( boost::iequals( sub.asString(), "structure" ) )
          {
            weapon.hitsStructures = true;
            weapon.hitsUnits = false;
          }
      if ( value.isMember( "filterExcludes" ) )
        for ( auto& sub : value["filterExcludes"] )
          if ( boost::iequals( sub.asString(), "ground" ) )
            weapon.hitsGround = false;
          else if ( boost::iequals( sub.asString(), "air" ) )
            weapon.hitsAir = false;
          else if ( boost::iequals( sub.asString(), "structure" ) )
            weapon.hitsStructures = false;

      if ( value.isMember( "effect" ) )
        weapon.fx = parseEffectData( value["effect"], weapon );
    }
  }

  void loadUnitData( const string& filename, UnitDataMap& unitMap )
  {
    unitMap.clear();

    Json::Value root = readJsonFile(filename);

    for ( auto it = root.begin(); it != root.end(); it++ )
    {
      UnitData entry;
      auto& unit = ( *it );

      auto& raceString = unit["race"].asString();
      entry.race = ( boost::iequals( raceString, "terran" ) ? Race::Terran : boost::iequals( raceString, "zerg" ) ? Race::Zerg : Race::Protoss );

      auto& key = it.key();
      auto id = _atoi64( key.asCString() );
      entry.id = id;
      entry.name = unit["name"].asString();

      entry.acceleration = unit["acceleration"].asFloat();
      entry.lifeRegenRate = unit["lifeRegenRate"].asFloat();
      entry.radius = unit["radius"].asFloat();
      entry.shieldRegenRate = unit["shieldRegenRate"].asFloat();
      entry.sight = unit["sight"].asFloat();
      entry.speed = unit["speed"].asFloat();
      entry.speedMultiplierCreep = unit["speedMultiplierCreep"].asFloat();
      entry.turningRate = unit["turningRate"].asFloat();

      entry.armored = unit["armored"].asBool();
      entry.biological = unit["biological"].asBool();
      entry.light = unit["light"].asBool();
      entry.massive = unit["massive"].asBool();
      entry.mechanical = unit["mechanical"].asBool();
      entry.flying = boost::iequals( unit["mover"].asString(), "Fly" ) ? true : false;
      entry.psionic = unit["psionic"].asBool();
      entry.structure = unit["structure"].asBool();

      entry.cargoSize = unit["cargoSize"].asInt();

      auto food = unit["food"].asFloat();
      if ( food < 0.0f )
      {
        entry.supplyCost = std::abs( food );
      }
      else if ( food > 0.0f )
      {
        entry.supplyProvided = food;
      }

      entry.lifeArmor = unit["lifeArmor"].asInt();
      entry.lifeMax = unit["lifeMax"].asInt();
      entry.lifeStart = unit["lifeStart"].asInt();
      entry.mineralCost = unit["mineralCost"].asInt();
      entry.scoreKill = unit["scoreKill"].asInt();
      entry.scoreMake = unit["scoreMake"].asInt();
      entry.shieldRegenDelay = unit["shieldRegenDelay"].asInt();
      entry.shieldsMax = unit["shieldsMax"].asInt();
      entry.shieldsStart = unit["shieldsStart"].asInt();
      entry.vespeneCost = unit["vespeneCost"].asInt();

      entry.resource = UnitData::Resource_None;

      auto& resource = unit["resource"];
      if ( resource.isArray() && resource.size() == 2 )
      {
        if ( boost::iequals( resource[0].asString(), "minerals" ) && boost::iequals( resource[1].asString(), "harvestable" ) )
          entry.resource = UnitData::Resource_MineralsHarvestable;
        else if ( boost::iequals( resource[0].asString(), "vespene" ) && boost::iequals( resource[1].asString(), "raw" ) )
          entry.resource = UnitData::Resource_GasBuildable;
        else if ( boost::iequals( resource[0].asString(), "vespene" ) && boost::iequals( resource[1].asString(), "harvestable" ) )
          entry.resource = UnitData::Resource_GasHarvestable;
        else
          HIVE_EXCEPT( "Unknown resource type " + resource[0].asString() + "/" + resource[1].asString() );
      }

      if ( unit["footprint"].isObject() && !unit["footprint"].empty() )
      {
        size_t width = 0;
        size_t height = 0;

        if ( unit["footprint"]["dimensions"].isArray() && unit["footprint"]["dimensions"].isValidIndex( 1 ) )
        {
          width = unit["footprint"]["dimensions"][0].asUInt64();
          height = unit["footprint"]["dimensions"][1].asUInt64();
        }

        if ( width > 0 && height > 0 )
        {
          entry.footprint.resize( width, height );
          entry.footprint.reset( UnitData::Footprint_Empty );
          string data = unit["footprint"]["data"].asString();
          if ( data.length() != ( width * height ) )
            HIVE_EXCEPT( filename + " footprint data length mismatches given dimensions" );

          entry.footprintOffset.x = unit["footprint"]["offset"][0].asInt();
          entry.footprintOffset.y = unit["footprint"]["offset"][1].asInt();

          for ( size_t y = 0; y < height; y++ )
            for ( size_t x = 0; x < width; x++ )
            {
              auto idx = y * width + x;
              entry.footprint[y][x] = (
                data[idx] == 'x' ? UnitData::Footprint_Reserved :
                data[idx] == 'n' ? UnitData::Footprint_NearResource :
                data[idx] == 'o' ? UnitData::Footprint_Creep :
                UnitData::Footprint_Empty );
            }
        }
      }

      for(auto& weapon : unit["weapons"])
      {
        entry.weapons.push_back(weapon.asCString());
      }

      unitMap[id] = entry;
    }
  }

  static void parseUpgradeRequirements(UpgradeInfo& upgrade, const Json::Value& value)
  {
    // Don't bother trying to parse the logic expressions. Just recurse all paths and take the one
    // value we care about (i.e. the id of the previous level of the upgrade).

    if(value["type"].asString() == "upgradeCount")
    {
      UpgradeID r = value["upgrade"].asInt();

      if(upgrade.target != r)
      {
        upgrade.upgradeRequirements.push_back(r);
      }
    }
    else
    {
      for(auto& operand : value["operands"])
      {
        parseUpgradeRequirements(upgrade, operand);
      }
    }
  };

  BuildAbility translateBuildAbility( UnitTypeID src, UnitTypeID dst, TechTreeRelationship& parsed )
  {
    auto& srcUnit = Database::unit( src );
    auto& dstUnit = Database::unit( dst );

    BuildAbility ret;
    ret.ability = parsed.ability;
    ret.unitCount = parsed.unitCount;
    ret.consumesSource = parsed.isMorph;
    if ( ret.consumesSource )
    {
      ret.mineralCost = std::max( 0, dstUnit.mineralCost - srcUnit.mineralCost ) * ret.unitCount;
      ret.vespeneCost = std::max( 0, dstUnit.vespeneCost - srcUnit.vespeneCost ) * ret.unitCount;
      ret.supplyCost = (int)( ( dstUnit.supplyCost * (Real)ret.unitCount ) - srcUnit.supplyCost );
    }
    else
    {
      ret.mineralCost = dstUnit.mineralCost * ret.unitCount;
      ret.vespeneCost = dstUnit.vespeneCost * ret.unitCount;
      ret.supplyCost = (int)( dstUnit.supplyCost * (Real)ret.unitCount );
    }

    return ret;
  }

  void TechTree::load( const string& filename )
  {
    upgrades_.clear();
    relationships_.clear();
    buildAbilities_.clear();

    Json::Value root = readJsonFile(filename);

    for ( auto itRace = root.begin(); itRace != root.end(); ++itRace )
    {
      auto& raceKey = itRace.key();
      auto raceName = raceKey.asCString();
      auto& race = *itRace;

      for ( auto itUnit = race.begin(); itUnit != race.end(); ++itUnit )
      {
        auto& unitKey = itUnit.key();
        auto unitName = (*itUnit)["name"].asString();
        auto& unit = *itUnit;
        uint32_t id = (uint32_t)_atoi64( unitKey.asCString() );

        auto parseBuild = [&](const char* skill, bool isMorph)
        {
          auto& builds = unit[skill];
          for ( auto& build : builds )
          {
            TechTreeRelationship relationship;
            relationship.source = id;
            relationship.ability = build["ability"].asUInt();
            relationship.time = build["time"].asFloat();
            relationship.isMorph = isMorph;
            relationship.name = build["unitName"].asCString();
            relationship.unitCount = 1;
            if ( build.isMember( "unitCount" ) && build["unitCount"].asInt() > 1 )
              relationship.unitCount = build["unitCount"].asInt();
            if ( build.isMember( "finishKillsWorker" ) && build["finishKillsWorker"].asBool() )
            {
              relationship.isMorph = true;
              relationship.morphCancellable = true;
            }
            if ( build.isMember( "finishKillsSource" ) && build["finishKillsWorker"].asBool() )
              relationship.isMorph = true;
            if ( build.isMember( "cancelKillsSource" ) && build["cancelKillsSource"].asBool() )
              relationship.morphCancellable = false;

            uint32_t target = build["unit"].asUInt();

            // only call this AFTER relationship has been otherwise filled
            // also, Database::unit() data has to exist
            buildAbilities_[std::make_pair( target, id )] = translateBuildAbility( id, target, relationship );

            for(auto& require : build["requires"])
            {
              auto type = require["type"].asString();
              if(type == "unitCount")
              {
                TechTreeUnitRequirement r;
                for(auto x : require["unit"])
                {
                  r.units.insert(x.asInt());
                }
                relationship.unitRequirements.push_back(r);
              }
            }

            relationships_.insert({ target, relationship });
          }
        };

        auto parseResearch = [&](const char* skill)
        {
          auto& builds = unit[skill];
          for ( auto& build : builds )
          {
            UpgradeInfo upgrade;
            upgrade.name = build["upgradeName"].asCString();
            upgrade.target = build["upgrade"].asUInt();
            upgrade.time = build["time"].asFloat();
            upgrade.ability = build["ability"].asUInt();
            upgrade.techBuilding = id;

            for(auto& require : build["requires"])
            {
              auto type = require["type"].asString();
              if(type == "unitCount")
              {
                TechTreeUnitRequirement r;
                for(auto x : require["unit"])
                {
                  r.units.insert(x.asInt());
                }
                upgrade.unitRequirements.push_back(r);
              }
              else
              {
                parseUpgradeRequirements(upgrade, require);
              }
            }

            upgrades_.insert({ upgrade.target, upgrade });
          }
        };

        parseBuild("builds", false);
        parseBuild("morphs", true);
        parseResearch("researches");
      }
    }
  }

  void TechTree::dump( Console* console ) const
  {
    console->printf( "Dumping buildAbilities_:" );
    for ( auto& it : buildAbilities_ )
    {
      auto src = it.first.second;
      auto dst = it.first.first;
      console->printf( "%s from %s:", sc2::UnitTypeToName( dst ), sc2::UnitTypeToName( src ) );
      auto& data = ( it.second );
      console->printf( "- ability: %d", data.ability );
      console->printf( "- unitCount: %d", data.unitCount );
      console->printf( "- consumesSource: %s", data.consumesSource ? "true" : "false" );
      console->printf( "- mineralCost: %d", data.mineralCost );
      console->printf( "- vespeneCost: %d", data.vespeneCost );
      console->printf( "- supplyCost: %d", data.supplyCost );
    }
  }

  void TechTree::findTechChain( UnitTypeID nonNormalizedTarget, vector<UnitTypeID>& chain) const
  {
    UnitTypeID target = normalizeUnitType(nonNormalizedTarget);
    if(target != nonNormalizedTarget)
    {
      return findTechChain(target, chain);
    }

    if(target == sc2::UNIT_TYPEID::TERRAN_SCV ||
      target == sc2::UNIT_TYPEID::ZERG_DRONE ||
      target == sc2::UNIT_TYPEID::PROTOSS_PROBE)
    {
      return;
    }

    auto iteratorPair = relationships_.equal_range(target);
    for(auto it = make_reverse_iterator(iteratorPair.second), endIt = make_reverse_iterator(iteratorPair.first); it != endIt; ++it)
    {
      auto& relationship = it->second;

      if(target == normalizeUnitType(relationship.source))
      {
        // Skip relationships that convert unit's special form back to its normalized form, e.g. unburrow.
        continue;
      }

      for(auto& r : relationship.unitRequirements)
      {
        if(r.units.empty())
        {
          continue;
        }

        bool found = false;
        for(auto& u : r.units)
        {
          if(std::find(chain.begin(), chain.end(), u) != chain.end())
          {
            found = true;
          }
        }

        if(!found)
        {
          // One of the requirements is needed, pick the first one.
          auto& unit = *r.units.begin();
          chain.push_back(unit);
          findTechChain(unit, chain);
        }
      }

      if(std::find(chain.begin(), chain.end(), relationship.source) != chain.end())
      {
        break; // Only one of the source buildings is needed, and its already in chain.
      }

      chain.push_back(relationship.source);
      findTechChain(relationship.source, chain);

      break; // Only one of the source buildings is needed, pick the first for now.
    }
  }

  UpgradeInfo TechTree::findTechChain(UpgradeID target, vector<UnitTypeID>& chain) const
  {
    auto iteratorPair = upgrades_.equal_range(target);
    for(auto it = make_reverse_iterator(iteratorPair.second), endIt = make_reverse_iterator(iteratorPair.first); it != endIt; ++it)
    {
      auto& upgrade = it->second;

      for(auto& r : upgrade.unitRequirements)
      {
        if(r.units.empty())
        {
          continue;
        }

        bool found = false;
        for(auto& u : r.units)
        {
          if(std::find(chain.begin(), chain.end(), u) != chain.end())
          {
            found = true;
          }
        }

        if(!found)
        {
          // One of the requirements is needed, pick the first one.
          auto& unit = *r.units.begin();
          chain.push_back(unit);
          findTechChain(unit, chain);
        }
      }

      if(std::find(chain.begin(), chain.end(), upgrade.techBuilding) == chain.end())
      {
        chain.push_back(upgrade.techBuilding);
        findTechChain(upgrade.techBuilding, chain);
      }

      // Only one of the source buildings is needed, pick the first for now.
      return upgrade;
    }

    assert(false && "Upgrade not found");

    return UpgradeInfo();
  }

  void resolveWeapons( UnitDataMap& units, WeaponDataMap& weapons )
  {
    for ( auto& unit : units )
    {
      for ( auto& name : unit.second.weapons )
      {
        if ( name.empty() )
          continue;
        if ( weapons.find( name ) == weapons.end() )
        {
          // HIVE_EXCEPT( "Weapon not found: " + name );
          continue;
        }
        auto& weapon = weapons[name];
        unit.second.resolvedWeapons_.push_back( &weapon );
      }
    }
  }

  void Database::load( const string& dataPath )
  {
    loadWeaponData( dataPath + R"(\weapons.json)", weaponData_ );
    loadUnitData( dataPath + R"(\units.json)", unitData_ );
    resolveWeapons( unitData_, weaponData_ );
    // load techtree AFTER all other data!
    techTree_.load( dataPath + R"(\techtree.json)" );
  }

  static const char* attribNames[(size_t)Attribute::Invalid] = {
    "None",
    "Light",
    "Armored",
    "Biological",
    "Mechanical",
    "Robotic",
    "Psionic",
    "Massive",
    "Structure",
    "Hover",
    "Heroic",
    "Summoned"
  };

  void dumpEffect( size_t indent, const WeaponEffectData& fx, int times )
  {
    string indstr;
    for ( size_t i = 0; i < indent; i++ )
      indstr.append( "  " );

    if ( fx.damage_ > 0.0f ) {
      printf( "%ssearches: %s%s%s%s\r\n", indstr.c_str(), fx.hitsGround_ ? "ground " : "", fx.hitsAir_ ? "air " : "", fx.hitsStructures_ ? "structures " : "", fx.hitsUnits_ ? "units" : "" );
      if ( times > 1 )
        printf( "%sdamage: %.2f * %i\r\n", indstr.c_str(), fx.damage_, times );
      else
        printf( "%sdamage: %.2f\r\n", indstr.c_str(), fx.damage_ );
      printf( "%sarmor reduction: %.2f\r\n", indstr.c_str(), fx.armorReduction_ );
    }
    else
      indent--;
    for ( size_t i = 0; i < (size_t)Attribute::Invalid; i++ )
      if ( fx.attributeBonuses_[i] > 0.0f )
        printf( "%s+ %.2f against %s\r\n", indstr.c_str(), fx.attributeBonuses_[i], attribNames[i] );
    if ( !fx.splash_.empty() )
    {
      printf( "%ssplash:\r\n", indstr.c_str() );
      for ( auto& splash : fx.splash_ )
        printf( "%s  %i%% at %f radius\r\n", indstr.c_str(), (int)(splash.fraction_ * 100.0f), splash.radius_ );
    }
    for ( auto& sub : fx.sub_ )
      dumpEffect( indent + 1, sub, fx.persistentHitCount_ );
  }

  void Database::dumpWeapons()
  {
    printf( "Weapons:\r\n" );
    for ( auto& weapon : weaponData_ )
    {
      printf( "  %s\r\n", weapon.second.name.c_str() );
      printf( "  hits: %s%s%s%s\r\n", weapon.second.hitsGround ? "ground " : "", weapon.second.hitsAir ? "air " : "", weapon.second.hitsStructures ? "structures " : "", weapon.second.hitsUnits ? "units" : "" );
      dumpEffect( 2, weapon.second.fx, weapon.second.fx.persistentHitCount_ );
    }
    system( "pause" );
  }

  inline Real fxAttributeBonusesAgainst( const WeaponEffectData& fx, const UnitData& unit )
  {
    Real damage = 0.0f;
    if ( unit.light )
      damage += fx.attributeBonuses_[(size_t)Attribute::Light];
    if ( unit.armored )
      damage += fx.attributeBonuses_[(size_t)Attribute::Armored];
    if ( unit.biological )
      damage += fx.attributeBonuses_[(size_t)Attribute::Biological];
    if ( unit.mechanical )
      damage += fx.attributeBonuses_[(size_t)Attribute::Mechanical];
    // missing: robotic!
    if ( unit.psionic )
      damage += fx.attributeBonuses_[(size_t)Attribute::Psionic];
    if ( unit.massive )
      damage += fx.attributeBonuses_[(size_t)Attribute::Massive];
    if ( unit.structure )
      damage += fx.attributeBonuses_[(size_t)Attribute::Structure];
    // missing: hover!
    // missing: heroic!
    // missing: summoned!
    return damage;
  }

  Real weaponFxSimpleSplash( const Real baseDamage, const vector<WeaponEffectSplashData>& data, Real& radius_out )
  {
    if ( data.empty() )
    {
      radius_out = 0.0f;
      return 0.0f;
    }

    Real cumulative = 0.0f;
    Real maxRadius = 0.0f;
    for ( auto& splash : data )
    {
      cumulative += ( baseDamage * splash.fraction_ );
      if ( splash.radius_ > maxRadius )
        maxRadius = splash.radius_;
    }
    return ( cumulative / (Real)data.size() ); // Return average damage
  }

  Real weaponFxSimpleDamage( const WeaponEffectData& fx, const UnitData& against, Real& damageTime_out, Real& splashRange_out, int armor = 0 )
  {
    // Time taken per attack
    Real time = 0.0f;
    // Max splash radius
    Real splashRadius = 0.0f;
    // Take base damage
    Real damage = fx.damage_;
    // Recurse through sub-effects
    Real subs = 0.0f;
    if ( !fx.isPersistent_ )
    {
      // Normal sub-effects: add all, divide by count (?)
      for ( auto& sub : fx.sub_ )
      {
        Real timeTemp = 0.0f;
        Real splashRangeTemp = 0.0f;
        subs += weaponFxSimpleDamage( sub, against, timeTemp, splashRangeTemp, armor );
        time += timeTemp;
        if ( splashRangeTemp > splashRadius )
          splashRadius = splashRangeTemp;
      }
      // I have no idea if this is correct, but it's an unusual situation anyway
      if ( fx.sub_.size() > 1 )
        subs /= (Real)fx.sub_.size();
    }
    else
    {
      // Still lacking: InitialEffect, ExpireEffect
      // Persistent effects: count time + hits
      int hitCount = std::max( 1, fx.persistentHitCount_ );
      size_t periodIndex = 0;
      for ( int i = 0; i < hitCount; i++ )
      {
        if ( periodIndex == fx.persistentPeriods_.size() - 1 )
          periodIndex = 0;
        time += fx.persistentPeriods_[periodIndex];
        for ( auto& sub : fx.sub_ )
        {
          Real timeTemp = 0.0f;
          Real splashRangeTemp = 0.0f;
          subs += weaponFxSimpleDamage( sub, against, timeTemp, splashRangeTemp, armor );
          time += timeTemp;
          if ( splashRangeTemp > splashRadius )
            splashRadius = splashRangeTemp;
        }
      }
    }
    // Add all sub-damage
    damage += subs;
    // Plus attribute bonuses against each attribute of unit
    damage += fxAttributeBonusesAgainst( fx, against );
    // Minus armor reduction times armor level
    damage -= ( fx.armorReduction_ * (Real)armor );
    // Add average splash damage, if any, and get maximum splash radius
    Real splashAverageDamage = weaponFxSimpleSplash( damage, fx.splash_, splashRadius );
    damage += splashAverageDamage;
    // Set attack time return value
    damageTime_out = time;
    // Set splash range return value
    splashRange_out = splashRadius;
    return damage;
  }

  Real WeaponData::calculateDamageAgainst( const UnitData& against, Real& damageTime_out, Real& splashRange_out, int armor ) const
  {
    return weaponFxSimpleDamage( fx, against, damageTime_out, splashRange_out, armor );
  }

  Real weaponFxSimpleBonuses( const WeaponEffectData& fx, Attribute attrib )
  {
    Real bonus = fx.attributeBonuses_[(size_t)attrib];
    for ( auto& sub : fx.sub_ )
      bonus += weaponFxSimpleBonuses( sub, attrib );
    return bonus;
  }

  Real WeaponData::calculateAttributeBonuses( Attribute attrib ) const
  {
    return weaponFxSimpleBonuses( fx, attrib );
  }

}
