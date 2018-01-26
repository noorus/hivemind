#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"

namespace hivemind {

  Intelligence::Intelligence( Bot* bot ): Subsystem( bot )
  {
  }

  void EnemyIntelligence::reset()
  {
    alive_ = true;
    lastSeen_ = 0;
  }

  void Intelligence::gameBegin()
  {
    bot_->messaging().listen( Listen_Global, this );

    enemies_.clear();
    units_.clear();
    bases_.clear();
    structures_.clear();

    for ( auto& player : bot_->players().all() )
    {
      if ( player.isEnemy() )
      {
        bot_->console().printf( "Intelligence: Init enemy %llu", player.id() );
        enemies_[player.id()].reset();
      }
    }
  }

  const Real c_enemyBaseHeurDist = 20.0f;

  void updateStructureData( UnitRef structure, EnemyStructure& data )
  {
    //
  }

  EnemyBase* Intelligence::_findExistingBase( UnitRef structure )
  {
    EnemyBase* base = nullptr;
    Real bestDist = std::numeric_limits<Real>::max();
    auto pos = Vector2( structure->pos );
    auto units = bot_->observation().GetUnits( Unit::Enemy );
    for ( auto unit : units )
    {
      if ( unit->owner != structure->owner || !utils::isStructure( unit ) || unit->display_type == sc2::Unit::Snapshot )
        continue;

      auto& data = structures_[unit];
      if ( data.destroyed_ || !data.base_ )
        continue;
      auto dist = pos.squaredDistance( data.position_ );
      if ( dist < bestDist )
      {
        bestDist = dist;
        base = data.base_;
      }
    }
    return base;
  }

  void Intelligence::_seeStructure( EnemyIntelligence& enemy, UnitRef unit )
  {
    // auto base = _findExistingBase( unit );
  }

  void Intelligence::_seeUnit( EnemyIntelligence& enemy, UnitRef unit )
  {
    //
  }

  void Intelligence::_updateUnit( UnitRef unit )
  {
    //
  }

  void Intelligence::_structureDestroyed( EnemyStructure& unit )
  {
    if ( unit.destroyed_ )
      return;

    unit.destroyed_ = bot_->time();

    bot_->messaging().sendGlobal( M_Intel_StructureDestroyed, &unit );
  }

  void Intelligence::_updateStructure( UnitRef unit, const bool destroyed )
  {
    auto& data = structures_[unit];
    if ( !data.id_ )
    {
      // initialize new entry
      data.id_ = unit;
      data.player_ = unit->owner;
      data.position_ = unit->pos;
    }
    if ( ( destroyed || !unit->is_alive ) && !data.destroyed_ )
    {
      _structureDestroyed( data );
      return;
    }
  }

  void Intelligence::update( const GameTime time, const GameTime delta )
  {
    auto& units = bot_->observation().GetUnits( Unit::Enemy );
    for ( auto& unit : units )
    {
      if ( unit->display_type != sc2::Unit::Visible )
        continue;

      if ( utils::isStructure( unit ) )
        _updateStructure( unit );
      else
        _updateUnit( unit );
    }
  }

  void Intelligence::draw()
  {
    Point2D screenPosition = Point2D( 0.9f, 0.5f );
    const Point2D increment( 0.0f, 0.011f );
    for ( auto& build : structures_ )
    {
      char text[128];
      sprintf_s( text, 128, "%d %s%s", build.second.player_, sc2::UnitTypeToName( build.second.id_->unit_type ), build.second.destroyed_ ? " (destroyed)" : "" );
      bot_->debug().drawText( text, screenPosition, sc2::Colors::Red );
      screenPosition += increment;
    }
  }

  void Intelligence::onMessage( const Message& msg )
  {
    if ( msg.code == M_Global_UnitEnterVision && utils::isEnemy( msg.unit() ) )
    {
      auto& enemy = enemies_[msg.unit()->owner];
      if ( utils::isStructure( msg.unit() ) )
      {
        _seeStructure( enemy, msg.unit() );
      }
      else
      {
        _seeUnit( enemy, msg.unit() );
      }
      if ( enemy.lastSeen_ == 0 )
      {
        // this should be safe, closestLocation could only return false if the map's base location vector is empty
        if ( utils::isMainStructure( msg.unit() ) || bot_->map().closestLocation( msg.unit()->pos )->isStartLocation() )
        {
          bot_->messaging().sendGlobal( M_Intel_FoundPlayer, 2, (size_t)msg.unit()->owner, msg.unit() );
          // TODO separate baseSeen value etc.
          enemy.lastSeen_ = bot_->time();
        }
      }
    }
    else if ( msg.code == M_Global_UnitDestroyed && utils::isEnemy( msg.unit() ) )
    {
      if ( utils::isStructure( msg.unit() ) )
      {
        _updateStructure( msg.unit(), true );
      }
    }
  }

  void Intelligence::gameEnd()
  {
    bot_->messaging().remove( this );
  }

}