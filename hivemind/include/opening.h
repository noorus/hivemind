#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "ai_goals.h"
#include "ai_agent.h"
#include "messaging.h"
#include "hive_vector2.h"
#include "resource_type_id.h"

namespace hivemind {

  struct Opening
  {
    typedef pair<ResourceTypeID, size_t> OpeningItem;

    vector<OpeningItem> items;
    size_t progress = 0;
    const char* name = nullptr;
  };

  Opening getRandomOpening();
}
