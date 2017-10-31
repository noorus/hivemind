#include "stdafx.h"
#include "bot.h"
#include "exception.h"
#include "database.h"
#include "hive_array2.h"
#include "console.h"

#include <codecvt>

using sc2::Coordinator;

HIVE_DECLARE_CONVAR( screen_width, "Width of the launched game window.", 1920 );
HIVE_DECLARE_CONVAR( screen_height, "Height of the launched game window.", 1080 );

HIVE_DECLARE_CONVAR( data_path, "Path to the bot's data directory.", R"(..\data)" );

HIVE_DECLARE_CONVAR( map, "Name or path of the map to launch.", "Fractured Glacier" );

HIVE_DECLARE_CONVAR( update_delay, "Time delay between main loop updates in milliseconds.", 10 );

void testTechChain(hivemind::Console& console, sc2::UNIT_TYPEID targetType)
{
  std::vector<sc2::UnitTypeID> techChain;
  hivemind::Database::techTree().findTechChain(targetType, techChain);

  //float range = hivemind::Database::weapon((hivemind::UnitType64)targetType).range;

  console.printf("How to build unit %s:", sc2::UnitTypeToName(targetType));
  //console.printf("  range: %f", range);

  for(auto unitType : techChain)
  {
    console.printf("  %s", sc2::UnitTypeToName(unitType));
  }
}

void testTechChain(hivemind::Console& console, sc2::UPGRADE_ID targetType)
{
  std::vector<sc2::UnitTypeID> techChain;
  auto upgrade = hivemind::Database::techTree().findTechChain(targetType, techChain);

  console.printf("How to research upgrade %s:", sc2::UpgradeIDToName(targetType));
  console.printf("  name: %s", upgrade.name.c_str());
  console.printf("  from: %s", sc2::UnitTypeToName(upgrade.techBuilding) );
  console.printf("  ability: %s", sc2::AbilityTypeToName(upgrade.ability) );
  console.printf("  time: %f", upgrade.time );
  console.printf("  requires:");
  for(auto unitType : techChain)
  {
    console.printf("    %s", sc2::UnitTypeToName(unitType));
  }
  if(!upgrade.upgradeRequirements.empty())
  {
    console.printf("  requires upgrades:");
    for(auto upgradeType : upgrade.upgradeRequirements)
    {
      console.printf("    %s", sc2::UpgradeIDToName(upgradeType));
    }
  }
}

const unsigned int c_updateSleepTime = 10; // 10 milliseconds

const char* c_dataPath = R"(..\data)";

static std::string GetExePath()
{
    WCHAR windowsPath[MAX_PATH];

    DWORD length = GetModuleFileNameW(0, windowsPath, MAX_PATH);
    if (length > 0) {
        std::wstring_convert<std::codecvt_utf8_utf16<WCHAR>, WCHAR> convertor;
        return convertor.to_bytes(windowsPath);
    }

    return std::string();
}

static std::string getMapName()
{
#if 1
    //const char* c_mapName = "micro/HydraliskMicro.SC2Map";
    const char* c_mapName = "micro/RoachMicro.SC2Map";

    // Coordinator searches relative paths starting from "../../maps" which would be outside the repo.
    // Workaround by creating an absolute path.
    std::string result = GetExePath();
    result = result.substr(0, result.find_last_of("\\"));
    result = result.substr(0, result.find_last_of("\\"));
    result += "\\maps\\";

    result += c_mapName;
    return result;
#else
  return "Fractured Glacier"; // "Interloper LE";
#endif
}

int main( int argc, char* argv[] )
{
#ifndef _DEBUG
  try
  {
#endif

    Coordinator coordinator;

    if ( !coordinator.LoadSettings( argc, argv ) )
      HIVE_EXCEPT( "Failed to load settings" );

    hivemind::Bot::Options options;

    int i = 0;
    while ( i < argc )
    {
      if ( _stricmp( argv[i], "-exec" ) == 0 )
      {
        i++;
        if ( i < argc )
          options.executeCommands_.emplace_back( argv[i] );
      }
      i++;
    }

    hivemind::Console console;

    hivemind::Bot hivemindBot( console );

    hivemindBot.initialize( options );

    hivemind::g_Bot = &hivemindBot;

    hivemind::Database::load( g_CVar_data_path.as_s() );

#if 1
    testTechChain(console, sc2::UNIT_TYPEID::TERRAN_SIEGETANK);
    testTechChain(console, sc2::UNIT_TYPEID::TERRAN_SIEGETANKSIEGED);
    testTechChain(console, sc2::UNIT_TYPEID::ZERG_HYDRALISK);
    testTechChain(console, sc2::UNIT_TYPEID::ZERG_HYDRALISKBURROWED);
    testTechChain(console, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED);
    testTechChain(console, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    testTechChain(console, sc2::UNIT_TYPEID::TERRAN_BARRACKS);
    testTechChain(console, sc2::UNIT_TYPEID::TERRAN_BARRACKSFLYING);
    return 0;
#endif

    coordinator.SetRealtime( true );

    coordinator.SetWindowSize(
      g_CVar_screen_width.as_i(),
      g_CVar_screen_height.as_i()
    );

    coordinator.SetParticipants( {
      CreateParticipant( sc2::Zerg, &hivemindBot ),
      CreateComputer( sc2::Terran, sc2::Difficulty::Medium ),
//      CreateComputer( sc2::Terran, sc2::Difficulty::Medium )
    } );

    coordinator.LaunchStarcraft();
    if ( !coordinator.StartGame( g_CVar_map.as_s() ) )
      HIVE_EXCEPT( "Failed to start game" );

    while ( coordinator.Update() )
    {
      sc2::SleepFor( g_CVar_update_delay.as_i() );
    }

    hivemind::g_Bot = nullptr;
#ifndef _DEBUG
  }
  catch ( hivemind::Exception& e )
  {
    printf_s( "EXCEPTION: %s\n", e.getFullDescription().c_str() );
    return EXIT_FAILURE;
  }
#endif

  return EXIT_SUCCESS;
}