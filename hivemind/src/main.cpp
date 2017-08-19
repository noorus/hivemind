#include "stdafx.h"
#include "bot.h"
#include "exception.h"
#include "hive_array2.h"

using sc2::Coordinator;

const unsigned int c_updateSleepTime = 10; // 10 milliseconds
const char* c_mapName = "Bastion of the Conclave";

int main( int argc, char* argv[] )
{
#ifndef _DEBUG
  try
  {
#endif
    Coordinator coordinator;

    if ( !coordinator.LoadSettings( argc, argv ) )
      HIVE_EXCEPT( "Failed to load settings" );

    coordinator.SetRealtime( true );

    coordinator.SetWindowSize( 1920, 1080 );

    hivemind::Bot hivemindBot;

    coordinator.SetParticipants( {
      CreateParticipant( sc2::Zerg, &hivemindBot ),
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