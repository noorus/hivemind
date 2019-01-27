#include "stdafx.h"
#include "brain.h"
#include "brain_macro.h"
#include "ai_goals.h"
#include "bot.h"
#include "database.h"
#include "opening.h"

namespace hivemind {

  static Opening makeEmptyOpening()
  {
    Opening opening;
    opening.name = "Empty opening";
    return opening;
  }

  static Opening makeDoubleHatchLingRoach()
  {
    Opening opening;
    opening.name = "Double hatch ling roach";
    opening.items =
    {
      // Hatchery first
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_OVERLORD,        1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           4 },
      { sc2::UNIT_TYPEID::ZERG_HATCHERY,        1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           3 },
      { sc2::UNIT_TYPEID::ZERG_EXTRACTOR,       1 },
      { sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL,    1 },
      // Ling Roach (vs Protoss)
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_OVERLORD,        1 },
      { sc2::UNIT_TYPEID::ZERG_QUEEN,           1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_QUEEN,           1 },
      { sc2::UNIT_TYPEID::ZERG_ZERGLING,        2 },
      { sc2::UPGRADE_ID::ZERGLINGMOVEMENTSPEED, 1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           2 },
      // Second hatchery after queens.
      { sc2::UNIT_TYPEID::ZERG_HATCHERY,        1 },
      { sc2::UNIT_TYPEID::ZERG_QUEEN,           1 },
      { sc2::UNIT_TYPEID::ZERG_OVERLORD,        1 },
      { sc2::UNIT_TYPEID::ZERG_ZERGLING,        2 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           7 },
      { sc2::UNIT_TYPEID::ZERG_SPORECRAWLER,    1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_QUEEN,           1 },
      { sc2::UNIT_TYPEID::ZERG_OVERLORD,        1 },
      { sc2::UNIT_TYPEID::ZERG_SPORECRAWLER,    1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,          11 },
      { sc2::UNIT_TYPEID::ZERG_LAIR,            1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           4 },
      { sc2::UNIT_TYPEID::ZERG_SPORECRAWLER,    1 },
      { sc2::UNIT_TYPEID::ZERG_ZERGLING,        4 },
      { sc2::UNIT_TYPEID::ZERG_EXTRACTOR,       1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           2 },
      { sc2::UNIT_TYPEID::ZERG_ROACHWARREN,     1 },
      { sc2::UNIT_TYPEID::ZERG_EXTRACTOR,       2 },
    };

    return opening;
  }

  static Opening makeLingRush()
  {
    Opening opening;
    opening.name = "Ling rush";
    opening.items =
    {
      // Hatchery first
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_EXTRACTOR,       1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_OVERLORD,        1 },
      { sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL,    1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UNIT_TYPEID::ZERG_DRONE,           1 },
      { sc2::UPGRADE_ID::ZERGLINGMOVEMENTSPEED, 1 },
      { sc2::UNIT_TYPEID::ZERG_ZERGLING,        3 },
      { sc2::UNIT_TYPEID::ZERG_QUEEN,           1 },
      { sc2::UNIT_TYPEID::ZERG_ZERGLING,        1 },
      { sc2::UNIT_TYPEID::ZERG_OVERLORD,        1 },
      { sc2::UNIT_TYPEID::ZERG_ZERGLING,        3 },
      { sc2::UNIT_TYPEID::ZERG_ZERGLING,        3 },
    };

    return opening;
  }

  Opening getRandomOpening()
  {
    int r = utils::randomBetween(0, 10);
    if(r <= 2)
    {
      return makeEmptyOpening();
    }
    else if(r <= 4)
    {
      return makeLingRush();
    }
    else
    {
      return makeDoubleHatchLingRoach();
    }
  }
}
