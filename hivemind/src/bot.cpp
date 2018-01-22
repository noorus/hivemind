#include "stdafx.h"
#include "bot.h"
#include "utilities.h"
#include "database.h"
#include "controllers.h"

namespace hivemind {

  Bot* g_Bot = nullptr;

  static bool callbackCVARGodmode( ConVar* variable, ConVar::Value oldValue );
  static bool callbackCVARCostIgnore( ConVar* variable, ConVar::Value oldValue );

  HIVE_DECLARE_CONVAR_WITH_CB( cheat_godmode, "Cheat: Invulnerability.", false, callbackCVARGodmode );
  HIVE_DECLARE_CONVAR_WITH_CB( cheat_ignorecost, "Cheat: Ignore all resource cost checks.", false, callbackCVARCostIgnore );

  bool callbackCVARGodmode( ConVar* variable, ConVar::Value oldValue )
  {
    if ( !g_Bot )
      return true;

    if ( variable->as_b() && !oldValue.i )
      g_Bot->enableGodmodeCheat();

    return true;
  }

  bool callbackCVARCostIgnore( ConVar* variable, ConVar::Value oldValue )
  {
    if ( !g_Bot )
      return true;

    if ( variable->as_b() && !oldValue.i )
      g_Bot->enableCostIgnoreCheat();

    return true;
  }

  Bot::Bot( Console& console ):
    time_( 0 ),
    console_( console ),
    players_( this ),
    brain_( this ),
    messaging_( this ),
    map_( this ),
    workers_( this ),
    baseManager_( this ),
    intelligence_( this ),
    strategy_( this ),
    builder_( this ),
    trainer_( this ),
    vision_( this ),
    cheatCostIgnore_( false ),
    cheatGodmode_( false )
  {
    console_.setBot( this );
  }

  Bot::~Bot()
  {
  }

  // TODO move somewhere smart
  static GameTime nextCreepUpdate = 0;

  void Bot::initialize( const Options& opts )
  {
    options_ = opts;

    for ( auto& exec : options_.executeCommands_ )
    {
      console_.execute( exec, true );
    }
  }

  void Bot::OnGameStart()
  {
    time_ = 0;
    lastStepTime_ = 0;

    nextCreepUpdate = 0;

    observation_ = Observation();
    debug_.setForward( Debug() );
    query_ = Query();
    action_ = Actions();

    console_.gameBegin();

    messaging_.gameBegin();

    map_.rebuild();
    map_.gameBegin();

    workers_.gameBegin();

    players_.gameBegin();
    brain_.gameBegin();

    baseManager_.gameBegin();

    vision_.gameBegin();

    intelligence_.gameBegin();

    strategy_.gameBegin();

    builder_.gameBegin();
    trainer_.gameBegin();

    if ( g_CVar_cheat_godmode.as_b() )
      enableGodmodeCheat();
    if ( g_CVar_cheat_ignorecost.as_b() )
      enableCostIgnoreCheat();
  }

  void Bot::enableGodmodeCheat()
  {
    if ( cheatGodmode_ || !debug_.get() )
      return;

    console_.printf( "Cheat: Enabling god mode" );
    debug_.cheatGodMode();
    cheatGodmode_ = true;
  }

  void Bot::enableCostIgnoreCheat()
  {
    if ( cheatCostIgnore_ || !debug_.get() )
      return;

    console_.printf( "Cheat: Ignoring all resource cost checks" );
    debug_.cheatIgnoreResourceCost();
    cheatCostIgnore_ = true;
  }

  void Bot::OnStep()
  {
    observation_ = Observation();
    debug_.setForward( Debug() );
    query_ = Query();
    action_ = Actions();

    time_ = observation_->GetGameLoop();
    auto delta = ( time_ - lastStepTime_ );
    lastStepTime_ = time_;

    map_.update( time_ );

    vision_.update( time_, delta );

    ControllerBase::setActions( action_ );

    builder_.update( time_, delta );
    trainer_.update( time_, delta );

    baseManager_.update( time_, delta );
    messaging_.update( time_ );
    workers_.update( time_ );
    brain_.update( time_, delta );
    strategy_.update( time_, delta );

    action_->SendActions();

    players_.draw();
    brain_.draw();
    strategy_.draw();
    workers_.draw();
    builder_.draw();

    map_.draw();
    baseManager_.draw();

    for ( auto unit : observation_->GetUnits() )
    {
      if ( unit->is_selected && utils::isMine( unit ) )
      {
        char hex[16];
        sprintf_s( hex, 16, "%x", id( unit ) );
        string txt = string( hex ) + " " + sc2::UnitTypeToName( unit->unit_type );
        txt.append( " (" + std::to_string( unit->unit_type ) + ")\n" );
        for ( auto& order : unit->orders )
          txt.append( string( sc2::AbilityTypeToName( order.ability_id ) ) + "\n" );
        debug_.drawText( txt, Vector3( unit->pos ), sc2::Colors::Green );
        MapPoint2 coord( unit->pos );
        auto regIndex = map_.regionMap_[coord.x][coord.y];
        string nrg = "region: " + std::to_string( regIndex );
        Vector3 nrgpos( unit->pos.x, unit->pos.y, unit->pos.z + 1.0f );
        debug_.drawText( nrg, nrgpos, sc2::Colors::Teal );
      }
    }

    debug_.send();
  }

  void Bot::OnGameEnd()
  {
    strategy_.gameEnd();
    brain_.gameEnd();
    builder_.gameEnd();
    trainer_.gameEnd();
    players_.gameEnd();
    workers_.gameEnd();
    vision_.gameEnd();
    baseManager_.gameEnd();
    intelligence_.gameEnd();
    messaging_.gameEnd();
    console_.gameEnd();
  }

  void Bot::OnUnitCreated( const Unit* unit )
  {
    console_.printf( "Bot::UnitCreated %s %p", sc2::UnitTypeToName( unit->unit_type ), unit );
    messaging_.sendGlobal( M_Global_UnitCreated, unit );
  }

  void Bot::OnUnitDestroyed( const Unit* unit )
  {
    console_.printf( "Bot::UnitDestroyed %s %p", sc2::UnitTypeToName( unit->unit_type ), unit );
    messaging_.sendGlobal( M_Global_UnitDestroyed, unit );
  }

  void Bot::OnUnitIdle( const Unit* unit )
  {
    messaging_.sendGlobal( M_Global_UnitIdle, unit );
  }

  void Bot::OnUnitEnterVision( const Unit* unit )
  {
    messaging_.sendGlobal( M_Global_UnitEnterVision, unit );
  }

  void Bot::OnUpgradeCompleted( UpgradeID upgrade )
  {
    console_.printf( "Bot::UpgradeCompleted %s", sc2::UpgradeIDToName( upgrade ) );
    messaging_.sendGlobal( M_Global_UpgradeCompleted, (uint32_t)upgrade );
  }

  void Bot::OnBuildingConstructionComplete( const Unit* unit )
  {
    console_.printf( "Bot::ConstructionCompleted %s", sc2::UnitTypeToName( unit->unit_type ) );
    messaging_.sendGlobal( M_Global_ConstructionCompleted, unit );
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