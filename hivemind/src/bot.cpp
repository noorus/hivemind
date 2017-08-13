#include "stdafx.h"
#include "bot.h"

namespace hivemind {

  Bot::Bot(): time_( 0 ), console_( this ), players_( this ), brain_( this ), messaging_( this )
  {
  }

  Bot::~Bot()
  {
  }

  void Bot::OnGameStart()
  {
    time_ = 0;
    lastStepTime_ = 0;

    observation_ = Observation();

    console_.gameBegin();
    players_.gameBegin();
    brain_.gameBegin();
    messaging_.gameBegin();
  }

  void Bot::OnStep()
  {
    observation_ = Observation();
    debug_ = Debug();
    query_ = Query();
    action_ = Actions();

    time_ = observation_->GetGameLoop();
    auto delta = ( time_ - lastStepTime_ );
    lastStepTime_ = time_;

    brain_.update( time_, delta );
    messaging_.update( time_ );

    players_.draw();
    brain_.draw();

    debug_->SendDebug();
  }

  void Bot::OnGameEnd()
  {
    messaging_.gameEnd();
    brain_.gameEnd();
    players_.gameEnd();
    console_.gameEnd();
  }

  void Bot::OnUnitDestroyed( const Unit& unit )
  {
    messaging_.sendGlobal( M_Global_UnitDestroyed, &unit );
  }

  void Bot::OnUnitCreated( const Unit& unit )
  {
    messaging_.sendGlobal( M_Global_UnitCreated, &unit );
  }

  void Bot::OnUnitIdle( const Unit& unit )
  {
    messaging_.sendGlobal( M_Global_UnitIdle, &unit );
  }

  void Bot::OnUpgradeCompleted( UpgradeID upgrade )
  {
    messaging_.sendGlobal( M_Global_UpgradeCompleted, (uint32_t)upgrade );
  }

  void Bot::OnBuildingConstructionComplete( const Unit& unit )
  {
    messaging_.sendGlobal( M_Global_UpgradeCompleted, &unit );
  }

  void Bot::OnNydusDetected()
  {
    console_.printf( "GLOBAL: Nydus detected!" );
    messaging_.sendGlobal( M_Global_NydusDetected );
  }

  void Bot::OnNuclearLaunchDetected()
  {
    console_.printf( "GLOBAL: Nuclear launch detected!" );
    messaging_.sendGlobal( M_Global_NuclearLaunchDetected );
  }

  void Bot::OnUnitEnterVision( const Unit& unit )
  {
    messaging_.sendGlobal( M_Global_UnitEnterVision, &unit );
  }

  const char* c_clientErrorText[11] = {
    "ErrorSC2",
    "An ability was improperly mapped to an ability id that doesn't exist.",
    "The response does not contain a field that was expected.",
    "The unit does not have any abilities.",
    "A request was made without consuming the response from the previous request, that puts this library in an illegal state.",
    "The response received from SC2 does not match the request.",
    "The websocket connection has prematurely closed, this could mean starcraft crashed or a websocket timeout has occurred.",
    "SC2UnknownStatus",
    "SC2 has either crashed or been forcibly terminated by this library because it was not responding to requests.",
    "The response from SC2 contains errors, most likely meaning the API was not used in a correct way.",
    "A request was made and a response was not received in the amount of time given by the timeout."
  };

  void Bot::OnError( const std::vector<ClientError>& client_errors, const std::vector<std::string>& protocol_errors )
  {
    for ( auto& error : client_errors )
      console_.printf( "CLIENT ERROR: %d - %s", error, c_clientErrorText[(int)error] );
    for ( auto& error : protocol_errors )
      console_.printf( "PROTOCOL ERROR: %s", error.c_str() );
  }

}