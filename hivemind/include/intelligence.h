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
    UnitRef id_;
    PlayerID player_;
    GameTime firstSeen_;
    GameTime lastSeen_;
    GameTime died_;
    Vector2 lastPosition_;
    explicit EnemyUnit( UnitRef id, PlayerID player, const Vector2& position, GameTime time ):
    id_( id ), player_( player ), lastPosition_( position ), firstSeen_( time ), lastSeen_( time ), died_( 0 )
    {
    }
  };

  using EnemyUnitMap = std::map<UnitRef, EnemyUnit>;

  struct EnemyStructure {
    UnitRef id_; // init once
    PlayerID player_; // init once
    Vector2 position_; // init once
    GameTime destroyed_;
    EnemyBase* base_;
    GameTime lastSeen_;
    EnemyStructure(): id_( nullptr ), player_( 0 ), destroyed_( 0 ), base_( nullptr ) {}
  };

  using EnemyStructureMap = std::map<UnitRef, EnemyStructure>;

  struct EnemyIntelligence {
    bool alive_;
    GameTime lastSeen_;
    void reset();
    EnemyIntelligence() { reset(); }
  };

  using EnemyMap = std::map<PlayerID, EnemyIntelligence>;

  struct EnemyRegionPresence {
    size_t structureCount_;
    void reset()
    {
      structureCount_ = 0;
    }
    EnemyRegionPresence() { reset(); }
  };

  using EnemyRegionPresenceMap = std::map<int, EnemyRegionPresence>;

  class InfluenceMap {
  private:
    Array2<Real> influence_;
  public:
    Bot * bot_;
  public:
    InfluenceMap( Bot* bot );
    ~InfluenceMap();
    void draw();
    void update( EnemyRegionPresenceMap& regions );
    void gameBegin();
  };

  class Intelligence: public Subsystem, Listener {
  private:
    EnemyMap enemies_;
    EnemyUnitMap units_;
    EnemyBaseVector bases_;
    EnemyStructureMap structures_;
    InfluenceMap influence_;
    EnemyRegionPresenceMap regions_;
    EnemyBase* _findExistingBase( UnitRef structure );
    void _seeStructure( EnemyIntelligence& enemy, UnitRef unit );
    void _seeUnit( EnemyIntelligence& enemy, UnitRef unit );
    void _updateStructure( UnitRef unit, const bool destroyed = false );
    void _updateUnit( UnitRef unit );
    void _structureDestroyed( EnemyStructure& unit );
  public:
    Intelligence( Bot* bot );
    inline EnemyMap& enemies() { return enemies_; }
    inline EnemyUnitMap& units() { return units_; }
    void gameBegin() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
    void onMessage( const Message& msg ) final;
    void gameEnd() final;
  };

}