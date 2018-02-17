#include "stdafx.h"
#include "bot.h"
#include "utilities.h"
#include "database.h"
#include "controllers.h"
#include "utilities.h"
#include "pathfinding.h"

namespace hivemind {

  Bot* g_Bot = nullptr;

  static bool callbackCVARGodmode( ConVar* variable, ConVar::Value oldValue );
  static bool callbackCVARCostIgnore( ConVar* variable, ConVar::Value oldValue );
  static bool callbackCVARShowMap ( ConVar* variable, ConVar::Value oldValue );
  static bool callbackCVARFastBuild( ConVar* variable, ConVar::Value oldValue );
  static void concmdGetMoney( Console* console, ConCmd* command, StringVector& arguments );
  static void concmdKill( Console* console, ConCmd* command, StringVector& arguments );

  HIVE_DECLARE_CONVAR_WITH_CB( cheat_godmode, "Cheat: Invulnerability.", false, callbackCVARGodmode );
  HIVE_DECLARE_CONVAR_WITH_CB( cheat_ignorecost, "Cheat: Ignore all resource cost checks.", false, callbackCVARCostIgnore );
  HIVE_DECLARE_CONVAR_WITH_CB( cheat_showmap, "Cheat: Reveal entire map and remove fog of war.", false, callbackCVARShowMap );
  HIVE_DECLARE_CONVAR_WITH_CB( cheat_fastbuild, "Cheat: Everything builds fast.", false, callbackCVARFastBuild);
  HIVE_DECLARE_CONCMD( cheat_getmoney, "Cheat: Get 5000 minerals and 5000 vespene.", concmdGetMoney );
  HIVE_DECLARE_CONCMD( kill, "Cheat: Kill selected units.", concmdKill );

  HIVE_DECLARE_CONVAR( map_always_hash, "Always hash the map file to obtain an identifier instead of recognizing Battle.net cache files.", false );

  HIVE_DECLARE_CONVAR( draw_units, "Whether to show debug information about the selected units.", true );

  HIVE_DECLARE_CONVAR( brain_enable, "Whether the brain is enabled and updated or not.", true );


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

  bool callbackCVARShowMap( ConVar* variable, ConVar::Value oldValue )
  {
    if ( !g_Bot )
      return true;

    if ( variable->as_b() && !oldValue.i )
      g_Bot->enableShowMapCheat();

    return true;
  }

  bool callbackCVARFastBuild( ConVar* variable, ConVar::Value oldValue )
  {
    if ( !g_Bot )
      return true;

    if ( variable->as_b() && !oldValue.i )
      g_Bot->enableFastBuildCheat();

    return true;
  }

  void concmdGetMoney( Console* console, ConCmd* command, StringVector& arguments )
  {
    if(!g_Bot)
      return;

    g_Bot->getMoneyCheat();
  }

  void concmdKill( Console* console, ConCmd* command, StringVector& arguments )
  {
    if ( !g_Bot || !g_Bot->state().inGame_ )
      return;

    for ( auto unit : g_Bot->observation().GetUnits() )
    {
      if ( unit->is_selected )
      {
        g_Bot->debug().cheatKillUnit( unit );
      }
    }
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
    vision_( this ),
    pathing_( this ),
    cheatCostIgnore_( false ),
    cheatGodmode_( false ),
    cheatShowMap_( false ),
    cheatFastBuild_( false )
  {
    console_.setBot( this );
  }

  Bot::~Bot()
  {
  }

  void Bot::initialize( const Options& opts )
  {
    options_ = opts;

    for ( auto& exec : options_.executeCommands_ )
    {
      console_.execute( exec, true );
    }

    state_.inGame_ = false;
    state_.action_ = Bot_GameOver;
    state_.brainInitialized_ = false;
  }

  void Bot::requestEndGame()
  {
    if ( state_.inGame_ )
    {
      state_.action_ = Bot_WantToQuit;
    }
  }

  void Bot::OnGameStart()
  {
    time_ = 0;
    lastStepTime_ = 0;

    state_.inGame_ = true;
    state_.action_ = Bot_Playing;

    platform::PerformanceTimer timer;
    timer.start();

    observation_ = Observation();
    debug_.setForward( Debug() );
    query_ = Query();
    action_ = Actions();

    console_.gameBegin();

    auto& gameInfo = observation_->GetGameInfo();
    console_.printf( "Map: %s (%dx%d)", gameInfo.map_name.c_str(),
      gameInfo.width, gameInfo.height );

    std::experimental::filesystem::path mapPath( gameInfo.local_map_path );

    MapData mapData;
    mapData.filepath = mapPath.string();

    bool gotHash = false;
    if ( !g_CVar_map_always_hash.as_b()
      && mapPath.stem().string().length() == 64
      && boost::iequals( mapPath.extension().string(), ".s2ma" ) )
    {
      if ( utils::hex2bin( mapPath.stem().string().c_str(), mapData.hash ) )
      {
        console_.printf( "Map: Assuming hash identifier from filename" );
        gotHash = true;
      }
    }

    if ( !gotHash )
      utils::readAndHashFile( mapData.filepath, mapData.hash );

    console_.printf( "Map: Hash %s", utils::hexString( mapData.hash ).c_str() );

    messaging_.gameBegin();

    map_.rebuild( mapData );
    map_.gameBegin();

    workers_.gameBegin();

    players_.gameBegin();

    if ( g_CVar_brain_enable.as_b() )
    {
      brain_.gameBegin();
      state_.brainInitialized_ = true;
    }

    baseManager_.gameBegin();

    vision_.gameBegin();

    intelligence_.gameBegin();

    strategy_.gameBegin();

    builder_.gameBegin();

    pathing_.gameBegin();

    console_.printf( "AI initialization took %.5fms", timer.stop() );

    cheatCostIgnore_ = false;
    cheatGodmode_ = false;
    cheatShowMap_ = false;
    cheatFastBuild_ = false;

    if ( g_CVar_cheat_godmode.as_b() )
      enableGodmodeCheat();
    if ( g_CVar_cheat_ignorecost.as_b() )
      enableCostIgnoreCheat();
    if ( g_CVar_cheat_showmap.as_b() )
      enableShowMapCheat();
    if ( g_CVar_cheat_fastbuild.as_b() )
      enableFastBuildCheat();
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

  void Bot::enableShowMapCheat()
  {
    if ( cheatShowMap_ || !debug_.get() )
      return;

    console_.printf( "Cheat: Removing fog of war" );
    debug_.cheatShowMap();
    cheatShowMap_ = true;
  }

  void Bot::enableFastBuildCheat()
  {
    if ( cheatFastBuild_ || !debug_.get() )
      return;

    console_.printf( "Cheat: Fast build" );
    debug_.cheatFastBuild();
    cheatFastBuild_ = true;
  }

  void Bot::getMoneyCheat()
  {
    if ( !debug_.get() )
      return;

    console_.printf( "Cheat: Get money" );
    debug_.cheatMoney();
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

    console_.executeBuffered();

    if ( state_.action_ == Bot_WantToQuit )
    {
      state_.action_ = Bot_Quitting;
      console_.print( "Ending current game by request" );
      debug_.endGame( false );
      action_->SendActions();
      debug_.send();
      return;
    }
    else if ( state_.action_ == Bot_Quitting )
    {
      action_->SendActions();
      debug_.send();
      return;
    }

    map_.update( time_ );

    vision_.update( time_, delta );

    intelligence_.update( time_, delta );

    ControllerBase::setActions( action_ );

    baseManager_.update( time_, delta );
    messaging_.update( time_ );
    workers_.update( time_ );
    builder_.update( time_, delta );

    bool doBrain = ( state_.brainInitialized_ && g_CVar_brain_enable.as_b() );

    if ( doBrain )
      brain_.update( time_, delta );

    strategy_.update( time_, delta );

    action_->SendActions();

    players_.draw();

    if ( doBrain )
      brain_.draw();

    strategy_.draw();
    workers_.draw();
    builder_.draw();
    intelligence_.draw();

    map_.draw();
    baseManager_.draw();

    static bool pathtest = false;
    if ( !pathtest )
    {
      pathtest = true;

      for ( int i = 0; i < 5; i++ )
      {
        auto idx = utils::randomBetween( 0, (int)map_.getBaseLocations().size() - 2 );
        auto path = pathing_.createPath( map_.getBaseLocations()[idx].position(), map_.getBaseLocations()[idx + 1].position() );
        console_.printf( "Pathing: Path from %d to %d - %d vertices%s", idx, idx + 1, path->verts().size(), path->verts().empty() ? " (NOT FOUND)" : "" );
      }

      //auto path = pathing_.createPath( {53, 100}, {73, 55});
      //auto path = pathing_.createPath( {53, 100}, {55, 100});
      //console_.printf( "Pathing: Path from {53, 100} to {73, 55} - %d vertices%s", path->verts().size(), path->verts().empty() ? " (NOT FOUND)" : "" );
    }

    pathing_.draw();

    if(g_CVar_draw_units.as_b())
    {
      for(auto unit : observation_->GetUnits())
      {
        if(unit->is_selected)
        {
          char hex[16];
          sprintf_s(hex, 16, "%x", id(unit));
          string txt = string(hex) + " " + sc2::UnitTypeToName(unit->unit_type);
          txt.append(" (" + std::to_string(unit->unit_type) + ")\n");
          if (utils::isMine(unit))
            for ( auto& order : unit->orders )
              txt.append( string( sc2::AbilityTypeToName( order.ability_id ) ) + "\n" );
          debug_.drawText(txt, Vector3(unit->pos), sc2::Colors::Green);

          MapPoint2 coord(unit->pos);
          auto regIndex = map_.regionMap_[coord.x][coord.y];
          string nrg = "region: " + std::to_string(regIndex);
          Vector3 nrgpos(unit->pos.x, unit->pos.y, unit->pos.z + 1.0f);
          debug_.drawText(nrg, nrgpos, sc2::Colors::Teal);
        }
      }
    }

    debug_.send();
  }

  void Bot::OnGameEnd()
  {
    strategy_.gameEnd();

    if ( state_.brainInitialized_ )
    {
      brain_.gameEnd();
      state_.brainInitialized_ = false;
    }

    pathing_.gameEnd();
    builder_.gameEnd();
    players_.gameEnd();
    workers_.gameEnd();
    vision_.gameEnd();
    baseManager_.gameEnd();
    intelligence_.gameEnd();
    messaging_.gameEnd();
    console_.gameEnd();

    state_.inGame_ = false;
    state_.action_ = Bot_GameOver;
  }

  void Bot::OnUnitCreated( const Unit* unit )
  {
    if ( unit->alliance == sc2::Unit::Self && unit->unit_type != UNIT_TYPEID::ZERG_DRONE && unit->unit_type != UNIT_TYPEID::ZERG_LARVA )
      console_.printf( "Bot::UnitCreated %s %x", sc2::UnitTypeToName( unit->unit_type ), id( unit ) );

    messaging_.sendGlobal( M_Global_UnitCreated, unit );
  }

  void Bot::OnUnitDestroyed( const Unit* unit )
  {
    console_.printf( "Bot::UnitDestroyed %s %x", sc2::UnitTypeToName( unit->unit_type ), id( unit ) );
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