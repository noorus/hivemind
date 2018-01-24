#include "stdafx.h"
#include "bot.h"
#include "exception.h"
#include "database.h"
#include "hive_array2.h"
#include "console.h"
#include "cache.h"

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

int runMain( hivemind::Bot::Options& options )
{
  hivemind::platform::initialize();

  hivemind::platform::prepareProcess();

#if 0
  hivemind::platform::Thread test( (HINSTANCE)0, "my_test_thread", [](
    hivemind::platform::Event& running, hivemind::platform::Event& wantStop, void* arg ) -> bool
  {
    OutputDebugStringA( "hello from thread\r\n" );
    OutputDebugStringA( "setting run\r\n" );
    running.set();
    while ( !wantStop.wait( 1000 ) )
    {
      OutputDebugStringA( "thread working!\r\n" );
    }
    OutputDebugStringA( "thread done, returning!\r\n" );
    return true;
  }, (void*)0xaabbccdd );
  OutputDebugStringA( "main: starting thread\r\n" );
  test.start();
  OutputDebugStringA( "main: waiting 10s\r\n" );
  Sleep( 10000 );
  OutputDebugStringA( "main: stopping thread\r\n" );
  test.stop();
  OutputDebugStringA( "main: done!\r\n" );

  return EXIT_SUCCESS;
#endif

#if 0
  hivemind::platform::ConsoleWindow wnd( "foobar", 100, 100, 200, 200 );

  while ( true )
    if ( !wnd.threadStep() )
      break;

  return EXIT_SUCCESS;
#endif

  Coordinator coordinator;

  char myExePath[MAX_PATH];
  strcpy_s( myExePath, MAX_PATH, options.hivemindExecPath_.c_str() );
  char* junkArgs[] = { myExePath };

  if ( !coordinator.LoadSettings( 1, junkArgs ) )
    HIVE_EXCEPT( "Failed to load settings" );

  hivemind::Console console;

  hivemind::Bot hivemindBot( console );

  hivemindBot.initialize( options );

  hivemind::g_Bot = &hivemindBot;

  hivemind::Cache::setBot( hivemind::g_Bot );

  hivemind::Database::load( g_CVar_data_path.as_s() );

#if 0
  hivemind::Database::dumpWeapons();
  return 0;
#endif

#if 0
  testTechChain( console, sc2::UNIT_TYPEID::TERRAN_SIEGETANK );
  testTechChain( console, sc2::UNIT_TYPEID::TERRAN_SIEGETANKSIEGED );
  testTechChain( console, sc2::UNIT_TYPEID::ZERG_HYDRALISK );
  testTechChain( console, sc2::UNIT_TYPEID::ZERG_HYDRALISKBURROWED );
  testTechChain( console, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED );
  testTechChain( console, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT );
  testTechChain( console, sc2::UNIT_TYPEID::TERRAN_BARRACKS );
  testTechChain( console, sc2::UNIT_TYPEID::TERRAN_BARRACKSFLYING );
  return 0;
#endif

  coordinator.SetRealtime( true );

  coordinator.SetWindowSize(
    g_CVar_screen_width.as_i(),
    g_CVar_screen_height.as_i()
  );

  coordinator.SetUseGeneralizedAbilityId( false );

  coordinator.SetParticipants( {
    CreateParticipant( sc2::Zerg, &hivemindBot ),
    CreateComputer( sc2::Terran, sc2::Difficulty::Medium ),
    CreateComputer( sc2::Terran, sc2::Difficulty::Medium )
    } );

  coordinator.LaunchStarcraft();
  if ( !coordinator.StartGame( g_CVar_map.as_s() ) )
    HIVE_EXCEPT( "Failed to start game" );

  while ( coordinator.Update() )
  {
    sc2::SleepFor( g_CVar_update_delay.as_i() );
  }

  hivemind::g_Bot = nullptr;

  hivemind::platform::shutdown();

  return EXIT_SUCCESS;
}

#ifdef HIVE_PLATFORM_WINDOWS

int APIENTRY wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
  UNREFERENCED_PARAMETER( hPrevInstance );
  UNREFERENCED_PARAMETER( lpCmdLine );
  UNREFERENCED_PARAMETER( nCmdShow );

  // Enable leak checking in debug builds
#if defined( _DEBUG ) || defined( DEBUG )
  _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

  // CRT memory allocation breakpoints can be set here
  // _CrtSetBreakAlloc( x );

  // Parse command line arguments into engine options
  int argCount;
  wchar_t** arguments = CommandLineToArgvW( lpCmdLine, &argCount );
  if ( !arguments )
    return EXIT_FAILURE;

  hivemind::Bot::Options options;

  if ( argCount > 0 )
  {
    options.hivemindExecPath_ = hivemind::platform::wideToUtf8( arguments[0] );

    int i = 1;
    while ( i < argCount )
    {
      if ( _wcsicmp( arguments[i], L"-exec" ) == 0 )
      {
        i++;
        if ( i < argCount )
          options.executeCommands_.emplace_back( hivemind::platform::wideToUtf8( arguments[i] ) );
      }
      i++;
    }
  }

  LocalFree( arguments );

  int retval = EXIT_FAILURE;

#ifndef _DEBUG
  try
  {
#endif

    retval = runMain( options );

#ifndef _DEBUG
  }
  catch ( hivemind::Exception& e )
  {
    MessageBoxA( 0, e.getFullDescription().c_str(), "Exception", MB_OK );
    return EXIT_FAILURE;
  }
  catch ( ... )
  {
    MessageBoxW( 0, L"Unknown exception", 0, MB_OK );
    return EXIT_FAILURE;
  }
#endif

  return retval;

}

#endif // HIVE_PLATFORM_WINDOWS