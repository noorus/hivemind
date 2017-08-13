#pragma once
#include "sc2_forward.h"
#include "console.h"
#include "player.h"
#include "brain.h"

namespace hivemind {

  class Bot: public sc2::Agent {
  protected:
    GameTime time_;
    GameTime lastStepTime_;
    const sc2::ObservationInterface* observation_;
    sc2::QueryInterface* query_;
    sc2::ActionInterface* action_;
    sc2::DebugInterface* debug_;
    Console console_;
    PlayerManager players_;
    Brain brain_;
  public:
    inline const GameTime time() const { return time_; }
    inline const sc2::ObservationInterface& observation() { return *observation_; }
    inline sc2::QueryInterface& query() { return *query_; }
    inline sc2::ActionInterface& action() { return *action_; }
    inline sc2::DebugInterface& debug() { return *debug_; }
    inline Console& console() { return console_; }
    inline PlayerManager& players() { return players_; }
    inline Brain& brain() { return brain_; }
  public:
    //! Callbacks
    virtual void OnGameStart() final;
    virtual void OnStep() final;
    virtual void OnGameEnd() final;
    virtual void OnUnitDestroyed( const Unit& unit ) final;
    virtual void OnUnitCreated( const Unit& unit ) final;
    virtual void OnUnitIdle( const Unit& unit ) final;
    virtual void OnUpgradeCompleted( UpgradeID upgrade ) final;
    virtual void OnBuildingConstructionComplete( const Unit& unit ) final;
    virtual void OnNydusDetected() final;
    virtual void OnNuclearLaunchDetected() final;
    virtual void OnUnitEnterVision( const Unit& unit ) final;
    virtual void OnError( const std::vector<ClientError>& client_errors, const std::vector<std::string>& protocol_errors = {} ) final;
  public:
    Bot();
    ~Bot();
  };

}