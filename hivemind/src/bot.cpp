#include "stdafx.h"
#include "bot.h"
#include "utilities.h"
#include "database.h"

namespace hivemind {

  const GameTime cCreepUpdateDelay = 40;

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

  void Bot::verifyData()
  {
    auto apiTypeData = observation_->GetUnitTypeData();

    vector<UnitTypeID> unitTypes = {
      sc2::UNIT_TYPEID::TERRAN_MARINE,
      sc2::UNIT_TYPEID::PROTOSS_ZEALOT,
      sc2::UNIT_TYPEID::ZERG_ZERGLING,
      sc2::UNIT_TYPEID::TERRAN_MARAUDER,
      sc2::UNIT_TYPEID::PROTOSS_STALKER,
      sc2::UNIT_TYPEID::ZERG_BANELING,
      sc2::UNIT_TYPEID::TERRAN_REAPER,
      sc2::UNIT_TYPEID::PROTOSS_ADEPT,
      sc2::UNIT_TYPEID::ZERG_ROACH,
      sc2::UNIT_TYPEID::PROTOSS_ARCHON,
      sc2::UNIT_TYPEID::ZERG_QUEEN,
      sc2::UNIT_TYPEID::PROTOSS_IMMORTAL
    };

    auto checki = []( int a, int b ) -> string
    {
      if ( a == b )
        return "ok";
      else
      {
        char asd[128];
        sprintf_s( asd, 128, "fail: %i (db) - %i (sc2)", a, b );
        return asd;
      }
    };

    auto checkf = []( Real a, Real b ) -> string
    {
      if ( math::abs( a - b ) < 0.0001f )
        return "ok";
      else
      {
        char asd[128];
        sprintf_s( asd, 128, "fail: %.4f (db) - %.4f (sc2)", a, b );
        return asd;
      }
    };

    for ( auto uid : unitTypes )
    {
      console_.printf( "Verifying: %s", UnitTypeToName( uid ) );
      auto sc2Data = apiTypeData[uid];
      auto dbData = Database::unit( uid );
      console_.printf( "- minerals: %s", checki( dbData.mineralCost, sc2Data.mineral_cost ).c_str() );
      console_.printf( "- vespene: %s", checki( dbData.vespeneCost, sc2Data.vespene_cost ).c_str() );
      console_.printf( "- speed: %s", checkf( dbData.speed, sc2Data.movement_speed ).c_str() );
      console_.printf( "- sight: %s", checkf( dbData.sight, sc2Data.sight_range ).c_str() );
      console_.printf( "- armor: %s", checkf( dbData.lifeArmor, sc2Data.armor ).c_str() );
      console_.printf( "- weapon count: %s", checki( dbData.weapons.size(), sc2Data.weapons.size() ).c_str() );
      // TODO: Try to figure out the same weapon(s) for comparison, by melee flags or range or so
      if ( !dbData.weapons.empty() && !sc2Data.weapons.empty() )
      {
        if ( dbData.weapons.size() < sc2Data.weapons.size() )
          console_.printf( "- wtf?! db has less weapons than sc2 (%d vs %d) - not parsing", dbData.weapons.size(), sc2Data.weapons.size() );
        else
        {
          // auto dbWpn = Database::weapon( uid );
          // auto sc2Wpn = sc2Data.weapons.back();
          for ( auto& sc2Wpn : sc2Data.weapons )
          {
            const WeaponData* dbWpn = nullptr;
            for ( auto& dbWpnName : dbData.weapons )
            {
              if ( !dbWpnName.empty() )
              {
                auto& wpn = Database::weapons().at( dbWpnName );
                if ( !dbWpn || math::abs( sc2Wpn.range - wpn.range ) < math::abs( sc2Wpn.range - dbWpn->range ) )
                  dbWpn = &wpn;
              }
            }
            if ( !dbWpn )
            {
              console_.printf( "--- wtf?! no db counterpart for weapon entry" );
            }
            else
            {
              console_.printf( "--- damage: %s", checkf( dbWpn->calculateBasicDamage(), sc2Wpn.damage_ ).c_str() );
              console_.printf( "--- range: %s", checkf( dbWpn->range, sc2Wpn.range ).c_str() );
              console_.printf( "--- period: %s", checkf( dbWpn->period, sc2Wpn.speed ).c_str() );
              // Don't bother checking attribute bonuses, since the API has all zeroes for now
              /*for ( size_t i = 0; i < (size_t)Attribute::Invalid; i++ )
              {
                auto dbVal = dbWpn->calculateAttributeBonuses( (Attribute)i );
                auto sc2Val = 0.0f;
                for ( auto& v : sc2Wpn.damage_bonus )
                  if ( v.attribute == (Attribute)i )
                    sc2Val = v.bonus;
                console_.printf( "--- attribute bonus %d: %s", i, checkf( dbVal, sc2Val ).c_str() );
              }*/
            }
          }
        }
      }
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

    workers_.gameBegin();

    players_.gameBegin();
    brain_.gameBegin();

    baseManager_.gameBegin();

    intelligence_.gameBegin();

    strategy_.gameBegin();

    builder_.gameBegin();
    trainer_.gameBegin();

    verifyData();

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

  static std::string GetAbilityText( sc2::AbilityID ability_id )
  {
    std::string str;
    str += sc2::AbilityTypeToName( ability_id );
    str += " (";
    str += std::to_string( uint32_t( ability_id ) );
    str += ")";
    return str;
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

    if ( time_ > nextCreepUpdate )
    {
      map_.updateCreep();
      map_.updateZergBuildable();
      nextCreepUpdate = time_ + cCreepUpdateDelay;
    }

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
      if ( unit->is_selected && utils::isMine( unit ) )
      {
        char hex[16];
        sprintf_s( hex, 16, "%x", id( unit ) );
        string txt = string( hex ) + " " + sc2::UnitTypeToName( unit->unit_type );
        txt.append( " (" + std::to_string( unit->unit_type ) + ")\n" );
        /*for ( auto& order : unit->orders )
          txt.append( GetAbilityText( order.ability_id ) + "\n" );*/
        debug_.drawText( txt, Vector3( unit->pos ), sc2::Colors::Green );
        MapPoint2 coord( unit->pos );
        auto regIndex = map_.regionMap_[coord.x][coord.y];
        string nrg = "region: " + std::to_string( regIndex );
        /*for ( size_t i = 0; i < map_.tempRegionPolygons_.size(); i++ )
        {
          if ( map_.tempRegionPolygons_[i].contains( unit->pos ) )
          {
            nrg.append( "inside polygon " + std::to_string( i ) + "\n" );
          }
        }*/
        Vector3 nrgpos( unit->pos.x, unit->pos.y, unit->pos.z + 1.0f );
        debug_.drawText( nrg, nrgpos, sc2::Colors::Teal );
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