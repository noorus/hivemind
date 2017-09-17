#pragma once
#include "sc2_forward.h"
#include "subsystem.h"

namespace hivemind {

  const PlayerID c_maxPlayers = 16;

  enum PlayerAlliance {
    Alliance_None = 0, //! Player is not in game
    Alliance_Self, //! Player is myself
    Alliance_Ally, //! Player is my ally
    Alliance_Enemy //! Player is my enemy
  };

  class Player {
  private:
    PlayerID id_;
    PlayerAlliance alliance_;
    PlayerType type_;
    Race race_;
    bool exists_;
  public:
    Player( const PlayerID& id );
    Player( const PlayerID& id, const Race& race, const PlayerType& type, const PlayerAlliance& alliance );
    const PlayerID id() const { return id_; }
    const PlayerAlliance alliance() const { return alliance_; }
    const Race race() const { return race_; }
    const bool ignored() const { return ( !exists_ || alliance_ == Alliance_None ); }
    const bool isMe() const { return ( alliance_ == Alliance_Self ); }
    const bool isAlly() const { return ( alliance_ == Alliance_Ally ); }
    const bool isEnemy() const { return ( alliance_ == Alliance_Enemy ); }
    const bool exists() const { return exists_; }
    const bool knownRace() const;
    inline const bool isTerran() const { return ( race_ == sc2::Terran ); }
    inline const bool isZerg() const { return ( race_ == sc2::Zerg ); }
    inline const bool isProtoss() const { return ( race_ == sc2::Protoss ); }
  };

  using PlayerVector = vector<Player>;

  class PlayerManager: public Subsystem {
  private:
    PlayerVector players_;
    PlayerID myID_;
  public:
    PlayerManager( Bot* bot );
    const Player& self() const;
    void gameBegin() final;
    void gameEnd() final;
    void draw() final;
  };

}