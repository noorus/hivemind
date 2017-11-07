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

      for(auto& effect : value["effect"])
      {
        // TODO: read effect.
      }
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
      entry.food = unit["food"].asInt();
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

  void TechTree::load( const string& filename )
  {
    upgrades_.clear();
    relationships_.clear();

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

            uint32_t target = build["unit"].asUInt();

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

  void Database::load( const string& dataPath )
  {
    loadWeaponData( dataPath + R"(\weapons.json)", weaponData_ );
    loadUnitData( dataPath + R"(\units.json)", unitData_ );
    techTree_.load( dataPath + R"(\techtree.json)" );
  }

}
