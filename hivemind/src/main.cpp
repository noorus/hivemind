#include "stdafx.h"
#include "bot.h"
#include "exception.h"
#include "database.h"
#include "hive_array2.h"
#include "console.h"

using sc2::Coordinator;

const unsigned int c_updateSleepTime = 10; // 10 milliseconds
const char* c_mapName = "Fractured Glacier"; // "Interloper LE";
const char* c_dataPath = R"(..\data)";

HIVE_DECLARE_CONVAR( screen_width, "Width of the launched game window.", 1920 );
HIVE_DECLARE_CONVAR( screen_height, "Height of the launched game window.", 1080 );

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

    hivemind::Database::load( c_dataPath );

    hivemind::Bot hivemindBot( options );

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
    if ( !coordinator.StartGame( c_mapName ) )
      HIVE_EXCEPT( "Failed to start game" );

    while ( coordinator.Update() )
    {
      sc2::SleepFor( c_updateSleepTime );
    }
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