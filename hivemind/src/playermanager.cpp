#include "stdafx.h"
#include "player.h"
#include "bot.h"

namespace hivemind {

  const char* c_raceNames[4] = {
    "Terran", "Zerg", "Protoss", "Random"
  };

  PlayerManager::PlayerManager( Bot* bot ): Subsystem( bot )
  {
    //
  }

  const Player & PlayerManager::self() const
  {
    return players_[myID_];
  }

  void PlayerManager::gameBegin()
  {
    gameEnd();
    for ( PlayerID i = 0; i < c_maxPlayers; i++ )
      players_.push_back( Player( i ) );

    myID_ = bot_->observation().GetPlayerID();
    auto gameInfo = &bot_->observation().GetGameInfo();
    for ( auto& player : gameInfo->player_info )
    {
      // FIXME This is hardcoded hack for now, the real data is not yet in the API
      PlayerAlliance alliance = ( player.player_id == myID_ ? Alliance_Self : Alliance_Enemy );
      players_[player.player_id] = Player( player.player_id, player.race_requested, player.player_type, alliance ); 
    }

    for ( auto& player : players_ )
    {
      if ( !player.exists() )
        continue;
      bot_->console().printf( "Player %d: %s %s %s",
        player.id(), player.isMe() ? "Hivemind" : "Computer",
        c_raceNames[player.race()],
        player.isMe() ? "Self" : player.isAlly() ? "Ally" : "Enemy" );
    }
  }

  void PlayerManager::gameEnd()
  {
    players_.clear();
  }

  void PlayerManager::draw()
  {
    Point2D pos( 0.03f, 0.05f );
    Real increment = 0.01f;

    bot_->debug().DebugTextOut( "PLAYERS", pos, sc2::Colors::White );
    pos.y += increment;

    for ( auto& player : players_ )
    {
      if ( !player.exists() )
        continue;

      auto color = ( player.isMe() ? sc2::Colors::Green : player.isAlly() ? sc2::Colors::Yellow : player.isEnemy() ? sc2::Colors::Red : sc2::Colors::Gray );

      char text[128];
      sprintf_s( text, 128, "%d %s %s %s",
        player.id(), player.isMe() ? "Hivemind" : "Computer",
        c_raceNames[player.race()],
        player.isMe() ? "Self" : player.isAlly() ? "Ally" : "Enemy" );
      bot_->debug().DebugTextOut( text, pos, color );

      pos.y += increment;
    }
  }

}