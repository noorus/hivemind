#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"

namespace hivemind {

  void loadUnitData( const string& filename, UnitDataMap& unitMap )
  {
    Json::Value root;
    std::ifstream infile;

    infile.open( filename, std::ifstream::in );

    if ( !infile.is_open() )
      HIVE_EXCEPT( "Failed to open units.json" );

    infile >> root;

    infile.close();

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

      unitMap[id] = entry;
    }
  }

  void TechTree::load( const string& filename )
  {
    relationships_.clear();

    Json::Value root;
    std::ifstream infile;

    infile.open( filename, std::ifstream::in );

    if ( !infile.is_open() )
      HIVE_EXCEPT( "Failed to open techtree.json" );

    infile >> root;

    infile.close();

    for ( auto& race : root )
    {
      for ( auto itUnit = race.begin(); itUnit != race.end(); itUnit++ )
      {
        auto& key = itUnit.key();
        auto id = _atoi64( key.asCString() );

        auto& builds = root["builds"];
        for ( auto& build : builds )
        {
          TechTreeRelationship relationship;
          relationship.source = (uint32_t)id;
          relationship.ability = build["ability"].asUInt();
          relationship.time = build["time"].asFloat();
          relationship.target = build["unit"].asUInt();
          relationship.isMorph = false;
          relationships_.push_back( relationship );
        }
      }
    }
  }

  void TechTree::findTechChain( UnitTypeID target ) 
  {
    // wat
  }

  UnitDataMap Database::unitData_;
  TechTree Database::techTree_;

  void Database::load()
  {
    unitData_.clear();
    loadUnitData( "../data/units.json", unitData_ );
    techTree_.load( "../data/techtree.json" );
  }

}