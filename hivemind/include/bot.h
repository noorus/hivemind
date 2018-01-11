#pragma once
#include "sc2_forward.h"
#include "console.h"
#include "player.h"
#include "brain.h"
#include "messaging.h"
#include "map.h"
#include "workers.h"
#include "basemanager.h"
#include "intelligence.h"
#include "utilities.h"
#include "strategy.h"
#include "builder.h"
#include "debug.h"
#include "trainer.h"

namespace hivemind {

  class Bot: public sc2::Agent {
  public:
    struct Options {
      StringVector executeCommands_; //!< Extra commands to execute on startup
    };
    std::map<UnitRef, std::string> unitDebugMsgs_;
  protected:
    GameTime time_;
    GameTime lastStepTime_;
    const sc2::ObservationInterface* observation_;
    sc2::QueryInterface* query_;
    sc2::ActionInterface* action_;
    DebugExtended debug_;
    Console& console_;
    PlayerManager players_;
    Messaging messaging_;
    Brain brain_;
    Map map_;
    WorkerManager workers_;
    BaseManager baseManager_;
    Intelligence intelligence_;
    Strategy strategy_;
    Builder builder_;
    Trainer trainer_;
    Options options_;
    bool cheatGodmode_;
    bool cheatCostIgnore_;
    void verifyData();
  public:
    inline const GameTime time() const { return time_; } //!< Game time in ticks
    inline const RealTime timeSeconds() const { return utils::ticksToTime( time_ ); } //!< Game time in ingame seconds
    inline const sc2::ObservationInterface& observation() { return *observation_; }
    inline sc2::QueryInterface& query() { return *query_; }
    inline sc2::ActionInterface& action() { return *action_; }
    inline DebugExtended& debug() { return debug_; }
    inline Console& console() { return console_; }
    inline PlayerManager& players() { return players_; }
    inline Messaging& messaging() { return messaging_; }
    inline Brain& brain() { return brain_; }
    inline Map& map() { return map_; }
    inline WorkerManager& workers() { return workers_; }
    inline BaseManager& bases() { return baseManager_; }
    inline Intelligence& intelligence() { return intelligence_; }
    inline Strategy& strategy() { return strategy_; }
    inline Builder& builder() { return builder_; }
    inline Trainer& trainer() { return trainer_; }
    void initialize( const Options& opts );
    void enableGodmodeCheat(); //!< This cannot be toggled back in the same game
    void enableCostIgnoreCheat(); //!< This cannot be toggled back in the same game
  public:
    //! Callbacks
    virtual void OnGameStart() final;
    virtual void OnStep() final;
    virtual void OnGameEnd() final;
    virtual void OnUnitCreated( const Unit* unit ) final;
    virtual void OnUnitDestroyed( const Unit* unit ) final;
    virtual void OnUnitIdle( const Unit* unit ) final;
    virtual void OnUpgradeCompleted( UpgradeID upgrade ) final;
    virtual void OnBuildingConstructionComplete( const Unit* unit ) final;
    virtual void OnNydusDetected() final;
    virtual void OnNuclearLaunchDetected() final;
    virtual void OnUnitEnterVision( const Unit* unit ) final;
    virtual void OnError( const std::vector<ClientError>& client_errors, const std::vector<std::string>& protocol_errors = {} ) final;
  public:
    Bot( Console& console );
    ~Bot();
  };

  extern Bot* g_Bot;

}
