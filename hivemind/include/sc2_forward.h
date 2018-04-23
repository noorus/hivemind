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

  using sc2::Attribute;

  using sc2::Color;

  using sc2::ActionInterface;

  namespace Colors {
    using namespace sc2::Colors;
  }

  //! \typedef UnitTag
  //! \brief Local type for referring to the unique ID of a specific unit in current game.
  //!   Tag is guaranteed to be unique for the lifetime of the current game.
  //!   The tag ID of a unit is accessible through \c unit->tag .
  //!   In most cases it makes more sense to use UnitRef directly instead.
  //! \deprecated Tags were used to refer to units before the SC2 client API was
  //!   redesigned to use direct, strongly guaranteed pointers to Units instead.
  //! \sa UnitRef
  using UnitTag = uint64_t;

  //! \typedef UnitRef
  //! \brief Local type for referring to a specific unit in current game.
  //!   Pointer is guaranteed unique and valid for the lifetime of the current game.
  //!   To see whether a unit is still alive, use \c unit->is_alive .
  using UnitRef = const Unit*;

  using UnitTypeIDSet = set<UnitTypeID>;

  using Point2Vector = vector<Point2D>;
  using Point3Vector = vector<Point3D>;

  using UnitSet = set<UnitRef>; //!< A set of specific units in current game.

  struct UnitVector: public vector<UnitRef> {
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