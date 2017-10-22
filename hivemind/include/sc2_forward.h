#pragma once

#include <sc2api/sc2_api.h>
#include <sc2lib/sc2_lib.h>
#include <sc2utils/sc2_manage_process.h>
#include <sc2renderer/sc2_renderer.h>
#include <vector>

#include "hive_types.h"
#include "hive_vector2.h"

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

  using sc2::ABILITY_ID;
  using sc2::AbilityID;

  using sc2::UPGRADE_ID;
  using sc2::UpgradeID;

  using UnitTag = uint64_t;
  using UnitRef = const Unit*;
  using UnitMap = std::map<UnitRef, sc2::UNIT_TYPEID>;

  using UnitTypeIDSet = set<UnitTypeID>;

  using Point2Vector = vector<Point2D>;
  using Point3Vector = vector<Point3D>;

  using BoolMap = vector<vector<bool>>;
  using IntMap = vector<vector<int>>;
  using FloatMap = vector<vector<float>>;
  using UnitSet = set<UnitRef>;

  struct UnitVector: public std::vector<UnitRef> {
  public:
    inline hivemind::Vector2 center() const
    {
      hivemind::Vector2 cntr( 0.0f, 0.0f );

      if ( empty() )
        return cntr;

      for ( auto unit : *this )
        cntr += hivemind::Vector2( unit->pos.x, unit->pos.y );

      return ( cntr / (Real)size() );
    }
  };

}