#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "player.h"
#include "baselocation.h"
#include "messaging.h"

namespace hivemind {

  struct EnemyBase {
    BaseLocation* location_;
    GameTime firstSeen_;
    GameTime lastSeen_;
    explicit EnemyBase( BaseLocation* location, GameTime time ):
    location_( location ), firstSeen_( time ), lastSeen_( time )
    {
    }
  };

  using EnemyBaseVector = vector<EnemyBase>;

  struct EnemyUnit {
    Tag id_;
    PlayerID player_;
    GameTime firstSeen_;
    GameTime lastSeen_;
    GameTime died_;
    Vector2 lastPosition_;
    explicit EnemyUnit( Tag id, PlayerID player, const Vector2& position, GameTime time ):
    id_( id ), player_( player ), lastPosition_( position ), firstSeen_( time ), lastSeen_( time ), died_( 0 )
    {
    }
  };
 
  using EnemyUnitMap = std::map<Tag, EnemyUnit>;

  struct EnemyIntelligence {
    bool alive_;
    GameTime lastSeen_;
    EnemyBaseVector bases_;
    EnemyIntelligence(): alive_( true ), lastSeen_( 0 ) {}
  };

  using EnemyMap = std::map<PlayerID, EnemyIntelligence>;

  class Intelligence: public Subsystem, Listener {
  private:
    EnemyMap enemies_;
    EnemyUnitMap units_;
  public:
    Intelligence( Bot* bot );
    inline EnemyMap& enemies() { return enemies_; }
    inline EnemyUnitMap& units() { return units_; }
    void gameBegin() final;
    void update( const GameTime time, const GameTime delta );
    void onMessage( const Message& msg ) final;
    void gameEnd() final;
  };

}