#include "stdafx.h"
#include "bot.h"
#include "exception.h"
#include "database.h"
#include "hive_array2.h"
#include "console.h"

using sc2::Coordinator;

HIVE_DECLARE_CONVAR( screen_width, "Width of the launched game window.", 1920 );
HIVE_DECLARE_CONVAR( screen_height, "Height of the launched game window.", 1080 );

HIVE_DECLARE_CONVAR( data_path, "Path to the bot's data directory.", R"(..\data)" );

HIVE_DECLARE_CONVAR( map, "Name or path of the map to launch.", "Fractured Glacier" );

HIVE_DECLARE_CONVAR( update_delay, "Time delay between main loop updates in milliseconds.", 10 );

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

    coordinator.SetRealtime( true );

    coordinator.SetWindowSize(
      g_CVar_screen_width.as_i(),
      g_CVar_screen_height.as_i()
    );

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