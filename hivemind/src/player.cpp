#include "stdafx.h"
#include "player.h"

namespace hivemind {

  Player::Player( const PlayerID& id ): id_( id ), race_( sc2::Race::Random ),
    type_( sc2::PlayerType::Observer ), alliance_( PlayerAlliance::Alliance_None ), exists_( false )
  {
  }

  Player::Player( const PlayerID& id, const Race& race, const PlayerType& type, const PlayerAlliance& alliance ) :
    id_( id ), race_( race ), type_( type ), exists_( true ), alliance_( alliance )
  {
  }

  const bool Player::knownRace() const
  {
    if ( alliance_ == Alliance_None )
      return true;

    return ( race_ != sc2::Race::Random );
  }

}