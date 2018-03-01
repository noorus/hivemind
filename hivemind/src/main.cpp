#include "stdafx.h"
#include "bot.h"
#include "exception.h"
#include "database.h"
#include "hive_array2.h"
#include "console.h"
#include "consolewindow_windows.h"
#include "cache.h"
#include "test_pathfinding.h"

using namespace hivemind;

static void concmdQuit( Console* console, ConCmd* command, StringVector& arguments );

HIVE_DECLARE_CONVAR( screen_width, "Width of the launched game window.", 1920 );
HIVE_DECLARE_CONVAR( screen_height, "Height of the launched game window.", 1080 );

HIVE_DECLARE_CONVAR( data_path, "Path to the bot's data directory.", R"(..\data)" );

HIVE_DECLARE_CONVAR( map, "Name or path of the map to launch.", "Fractured Glacier" );

HIVE_DECLARE_CONVAR( update_delay, "Time delay between main loop updates in milliseconds.", 10 );

HIVE_DECLARE_CONVAR( net_timeout, "Game API communications timeout in milliseconds.", 20000 );

HIVE_DECLARE_CONCMD( quit, "Quit currently active game.", concmdQuit );

void testTechChain(Console& console, sc2::UNIT_TYPEID targetType)
{
  std::vector<sc2::UnitTypeID> techChain;
  Database::techTree().findTechChain(targetType, techChain);

  //float range = hivemind::Database::weapon((hivemind::UnitType64)targetType).range;

  console.printf("How to build unit %s:", sc2::UnitTypeToName(targetType));
  //console.printf("  range: %f", range);

  for(auto unitType : techChain)
  {
    console.printf("  %s", sc2::UnitTypeToName(unitType));
  }
}

void concmdQuit( Console* console, ConCmd* command, StringVector& arguments )
{
  if ( !g_Bot || !g_Bot->state().inGame_ )
    return;

  g_Bot->requestEndGame();
}

void testTechChain(Console& console, sc2::UPGRADE_ID targetType)
{
  std::vector<sc2::UnitTypeID> techChain;
  auto upgrade = Database::techTree().findTechChain(targetType, techChain);

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

const string c_consoleThreadName = "hiveConsole";
const string c_consoleTitle = "hivemind//console";

int runMain( hivemind::Bot::Options& options )
{
  platform::initialize();

  platform::prepareProcess();

  Console console;

  platform::Thread consoleWindowThread( c_consoleThreadName,
    []( platform::Event& running, platform::Event& wantStop, void* argument ) -> bool
  {
    auto cnsl = (Console*)argument;
    platform::ConsoleWindow window( cnsl, c_consoleTitle, 220, 220, 640, 320 );
    running.set();
    window.messageLoop( wantStop );
    return true;
  }, &console );

  pathfinding::testPathfinding(&console);

  sc2::Coordinator coordinator;

  consoleWindowThread.start();

  console.executeFile( "user.cfg" );

  char myExePath[MAX_PATH];
  strcpy_s( myExePath, MAX_PATH, options.hivemindExecPath_.c_str() );
  char* junkArgs[] = { myExePath };

  if ( !coordinator.LoadSettings( 1, junkArgs ) )
    HIVE_EXCEPT( "Failed to load settings" );

  coordinator.SetTimeoutMS( g_CVar_net_timeout.as_i() );

  Bot hivemindBot( console );

  hivemindBot.initialize( options );

  hivemind::g_Bot = &hivemindBot;

  Cache::setBot( hivemind::g_Bot );

  Database::load( g_CVar_data_path.as_s() );

#if 0
  Database::techTree().dump( &console );
  Sleep( 100000 );
  return 0;
#endif

  coordinator.SetRealtime( true );
  coordinator.SetMultithreaded( false );

  coordinator.SetWindowSize(
    g_CVar_screen_width.as_i(),
    g_CVar_screen_height.as_i()
  );

  coordinator.SetUseGeneralizedAbilityId( false );

  coordinator.SetParticipants( {
    CreateParticipant( sc2::Zerg, &hivemindBot ),
    CreateComputer( sc2::Terran, sc2::Difficulty::VeryHard ),
    CreateComputer( sc2::Protoss, sc2::Difficulty::VeryHard )
    } );

  coordinator.LaunchStarcraft();

  if ( !coordinator.StartGame( g_CVar_map.as_s() ) )
    HIVE_EXCEPT( "Failed to start game" );

  while ( true )
  {
    if ( !consoleWindowThread.check() )
    {
      hivemindBot.requestEndGame();
    }
    if ( !coordinator.Update() )
    {
      coordinator.LeaveGame();
      break;
    }
    sc2::SleepFor( g_CVar_update_delay.as_i() );
  }

  coordinator.WaitForAllResponses();

  hivemind::g_Bot = nullptr;

  consoleWindowThread.stop();

  platform::shutdown();

  return EXIT_SUCCESS;
}

#ifdef HIVE_PLATFORM_WINDOWS

int APIENTRY wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
  UNREFERENCED_PARAMETER( hPrevInstance );
  UNREFERENCED_PARAMETER( lpCmdLine );
  UNREFERENCED_PARAMETER( nCmdShow );

  // Enable leak checking in debug builds
  // FIXME: Disabled for now because sc2::Coordinator dumps a shitload of memory for whatever reason and it's annoying
#if defined( _DEBUG ) || defined( DEBUG )
  // _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  // CRT memory allocation breakpoints can be set here
  // _CrtSetBreakAlloc( x );
#endif

  platform::g_instance = hInstance;

  // Parse command line arguments into engine options
  int argCount;
  wchar_t** arguments = CommandLineToArgvW( lpCmdLine, &argCount );
  if ( !arguments )
    return EXIT_FAILURE;

  Bot::Options options;

  wchar_t modulePath[MAX_PATH + 1];
  GetModuleFileNameW( nullptr, modulePath, MAX_PATH + 1 );
  options.hivemindExecPath_ = platform::wideToUtf8( modulePath );

  int i = 0;
  while ( i < argCount )
  {
    if ( _wcsicmp( arguments[i], L"-exec" ) == 0 )
    {
      ++i;
      if ( i < argCount )
        options.executeCommands_.emplace_back( platform::wideToUtf8( arguments[i] ) );
    }
    ++i;
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