#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "player.h"
#include "baselocation.h"
#include "messaging.h"
#include "hive_array2.h"

namespace hivemind {

  // Note: This must match values in sc2_client.cc ObservationImp::GetVisibility()
  enum VisibilityTile: uint8_t {
    Visibility_Hidden = 0,
    Visibility_Fogged,
    Visibility_Visible,
    Visibility_FullHidden
  };

  struct RegionVisibility {
    size_t tiles_; //!< Total tiles
    size_t visible_; //!< Visible tiles
    inline void reset() {
      tiles_ = 0;
      visible_ = 0;
    }
    inline Real coverage() const {
      if ( visible_ == 0 || tiles_ == 0 )
        return 0.0f;
      return ( (Real)visible_ / (Real)tiles_ );
    }
    RegionVisibility() { reset(); }
  };

  using RegionVisibilityMap = std::map<int, RegionVisibility>;

  class Vision: public Subsystem, Listener {
  public:
    struct StartLocations {
      set<size_t> unexplored_;
      set<size_t> explored_;
    };
  private:
    StartLocations startLocations_;
    Array2<VisibilityTile> visibility_; //!< Current visibility to the map
    RegionVisibilityMap regionVision_;
    GameTime lastVisUpdate_;
    bool updateVisibility();
  public:
    Vision( Bot* bot );
    void gameBegin() final;
    void update( const GameTime time, const GameTime delta );
    void onMessage( const Message& msg ) final;
    inline StartLocations& startLocations() { return startLocations_; }
    const RegionVisibilityMap& regionVision() const { return regionVision_; }
    void gameEnd() final;
  };

}