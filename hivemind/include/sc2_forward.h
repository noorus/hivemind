#pragma once

#include "hive_types.h"

#include <sc2api/sc2_api.h>
#include <sc2lib/sc2_lib.h>
#include <sc2utils/sc2_manage_process.h>
#include <sc2renderer/sc2_renderer.h>

namespace hivemind {

  using sc2::Unit;
  using sc2::UpgradeID;
  using sc2::ClientError;
  using sc2::UnitTypeID;
  using sc2::Race;

  using sc2::Point2D;
  using sc2::Point3D;
  using sc2::Point2DI;
  using sc2::GameInfo;
  using sc2::Visibility;

  using sc2::UNIT_TYPEID;
  using sc2::PlayerType;
  using sc2::Tag;

  using UnitTag = uint64_t;
  using UnitMap = std::map<Tag, sc2::UNIT_TYPEID>;

  using UnitVector = vector<Unit>;
  using Point2Vector = vector<Point2D>;
  using Point3Vector = vector<Point3D>;

  using BoolMap = vector<vector<bool>>;
  using IntMap = vector<vector<int>>;
  using FloatMap = vector<vector<float>>;
  using TagVector = vector<Tag>;
  using TagSet = set<Tag>;

}