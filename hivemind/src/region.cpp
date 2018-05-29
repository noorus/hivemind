#include "stdafx.h"
#include "map.h"
#include "bot.h"
#include "utilities.h"
#include "map_analysis.h"
#include "hive_geometry.h"
#include "baselocation.h"
#include "database.h"
#include "blob_algo.h"
#include "regiongraph.h"
#include "cache.h"
#include "region.h"

namespace hivemind {

  const CachedPath& Region::getChokePath( ChokepointID from, ChokepointID to ) const
  {
    assert( from != to );
    auto key = pair<ChokepointID, ChokepointID>( from, to );
    auto it = chokePaths_.find( key );
    assert( it != chokePaths_.end() );
    return it->second;
  }

}