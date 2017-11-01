#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "builder.h"

namespace hivemind {

  Builder::Builder( Bot* bot ): Subsystem( bot )
  {
  }

  void Builder::gameBegin()
  {
  }

  void Builder::gameEnd()
  {
  }

  Point2D Builder::findPlacement( UnitTypeID structure, const Base& base, BuildingPlacement type )
  {
    assert( type == BuildPlacement_Generic );

    if ( type == BuildPlacement_Generic )
    {
      UnitRef depot = nullptr;
      if ( !base.depots().empty() )
        depot = *base.depots().cbegin();

      Vector2 pos = ( depot ? depot->pos : base.location()->position() );

      auto& closestTiles = bot_->map().getDistanceMap( pos ).sortedTiles();

      for ( auto& tile : closestTiles )
      {
        if ( bot_->map().canZergBuild( structure, tile, 2 ) )
          return tile;
      }
    }

    return Point2D();
  }

}