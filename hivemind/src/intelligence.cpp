#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( threats_debug, "Whether to draw threat intel debug information.", 1 );

  Intelligence::Intelligence( Bot* bot ): Subsystem( bot ), influence_( bot )
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

    influence_.gameBegin();
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
    auto& data = structures_[unit];
    data.lastSeen_ = bot_->time();
  }

  void Intelligence::_seeUnit( EnemyIntelligence& enemy, UnitRef unit )
  {
    auto& data = units_[unit];
    data.lastSeen_ = bot_->time();
    data.lastPosition_ = unit->pos;
  }

  void Intelligence::_updateUnit( UnitRef unit, const bool killed )
  {
    auto& data = units_[unit];
    if ( !data.id_ )
    {
      // initialize new entry
      data.id_ = unit;
      data.player_ = unit->owner;
    }
    if ( ( killed || !unit->is_alive ) && !data.died_ )
    {
      data.died_ = bot_->time();
      return;
    }
    data.lastSeen_ = bot_->time();
    data.lastPosition_ = unit->pos;
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
    data.lastSeen_ = bot_->time();
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

    for ( auto& presence : regions_ )
      presence.second.reset();

    for ( auto& structure : structures_ )
    {
      if ( structure.second.destroyed_ )
        continue;
      auto region = bot_->map().region( structure.second.position_ );
      if ( !region )
        continue;
      regions_[region->label_].structureCount_++;
    }

    influence_.update( regions_ );
  }

  bool EnemyIntelligence::canUnitHit( UnitTypeID src, UnitTypeID dest, EnemyWeaponThreatVector& threats_out ) const
  {
    auto& enemy = Database::unit( src );
    auto& home = Database::unit( dest );

    EnemyWeaponThreatVector vec;

    for ( auto weapon : enemy.resolvedWeapons_ )
    {
      // these are totally fucking broken. WTF?
      //if ( home.structure && !weapon->hitsStructures )
      //  continue;
      //if ( !home.structure && !weapon->hitsUnits )
      //  continue;
      //if ( home.flying && !weapon->hitsAir )
      //  continue;
      //if ( !home.flying && !weapon->hitsGround )
      //  continue;

      EnemyWeaponThreat threat;
      threat.range = enemy.radius + weapon->range;
      threat.outrunRange = threat.range + weapon->rangeSlop;
      Real damageTime;
      int armor = 0; // dunno real armor level, no data kept yet
      Real splashRange = 0.0f;
      // calculateDamageAgainst gives us the time a periodic effect cycle lasts;
      // add to that the actual weapon usage period to get total time to go with total damage
      Real damage = weapon->calculateDamageAgainst( home, damageTime, splashRange, armor );
      damageTime += weapon->damagePoint + weapon->period;
      threat.dps = ( damageTime > 0.0f ? ( damage / damageTime ) : damage );
      threat.splashRange = splashRange;

      vec.push_back( threat );
    }

    std::swap( threats_out, vec );

    return ( !threats_out.empty() );
  }

  // ---

  InfluenceMap::InfluenceMap( Bot* bot ): bot_( bot )
  {
  }

  InfluenceMap::~InfluenceMap()
  {
  }

  void InfluenceMap::gameBegin()
  {
    influence_.resize( bot_->map().width(), bot_->map().height() );
    influence_.reset( 0.0f );
  }

  void InfluenceMap::update( EnemyRegionPresenceMap& regions )
  {
    influence_.reset( 0.0f );

    for ( size_t y = 0; y < influence_.height(); ++y )
    {
      for ( size_t x = 0; x < influence_.width(); ++x )
      {
        auto region = bot_->map().regionId( x, y );
        if ( region >= 0 )
        {
          influence_[x][y] += (Real)regions[region].structureCount_;
        }
      }
    }
  }

  void InfluenceMap::draw()
  {
    for ( size_t y = 0; y < influence_.height(); ++y )
    {
      for ( size_t x = 0; x < influence_.width(); ++x )
      {
        float influence = influence_[x][y];

        if ( influence != 0.0f )
        {
          sc2::Color color = influence > 0.0f ? sc2::Color( 255, 0, 0 ) : sc2::Color( 0, 0, 255 );

          auto pos = sc2::Point3D( float( x ), float( y ), 0.0f );
          pos.z = bot_->map().heightMap_[x][y] + 0.2f;

          bot_->debug().drawBox( pos, pos + sc2::Point3D( 1.0f, 1.0f, 0.0f ), color );

          string text = std::to_string( int( influence ) );
          bot_->debug().drawText( text, Vector3( pos + sc2::Point3D( 0.5f, 0.5f, 0.0f ) ), color );
        }
      }
    }
  }

  // ---

  void Intelligence::draw()
  {
    Point2D screenPosition = Point2D( 0.85f, 0.5f );
    const Point2D increment( 0.0f, 0.011f );
    for ( auto& build : structures_ )
    {
      char text[128];
      sprintf_s( text, 128, "%s%s", sc2::UnitTypeToName( build.second.id_->unit_type ), build.second.destroyed_ ? " (destroyed)" : "" );
      bot_->debug().drawText( text, screenPosition, sc2::Colors::Red );
      screenPosition += increment;
    }

    // influence_.draw();
    auto& fxdata = bot_->observation().GetEffectData();
    for ( auto& fx : bot_->observation().GetEffects() )
    {
      for ( auto& pos : fx.positions )
      {
        string name = "";
        float radius = 1.5f;
        for ( auto& data : fxdata )
        {
          if ( data.effect_id == fx.effect_id )
          {
            name = data.name;
            radius = data.radius;
          }
        }
        auto x = math::floor( pos.x );
        auto y = math::floor( pos.y );
        auto p = Vector3( pos.x, pos.y, bot_->map().heightMap_[x][y] + 0.25f );
        bot_->debug().drawSphere( p, radius, Colors::Green );
        p.z += 0.15f;
        char asd[128];
        sprintf_s( asd, 128, "effect %d (%s)", fx.effect_id, name.c_str() );
        bot_->debug().drawText( asd, p, Colors::Teal, 8 );
      }
    }

    if ( g_CVar_threats_debug.as_i() > 0 )
    {
      for ( auto& it : units_ )
      {
        auto& unit = it.second;
        if ( !unit.id_ || unit.died_ )
          continue;
        EnemyWeaponThreatVector threats;
        if ( enemies_[unit.player_].canUnitHit( unit.id_->unit_type, UNIT_TYPEID::ZERG_ZERGLING, threats ) )
        {
          Real zadd = 0.0f;
          for ( auto& threat : threats )
          {
            zadd += 0.1f;
            Real range = threat.range;
            Real outrunRange = threat.outrunRange;
            auto x = math::floor( unit.lastPosition_.x );
            auto y = math::floor( unit.lastPosition_.y );
            auto pos = unit.lastPosition_.to3( bot_->map().heightMap_[x][y] + zadd );
            bot_->debug().drawCircle( pos, outrunRange, Colors::Yellow );
            pos.z += 0.05f;
            bot_->debug().drawCircle( pos, range, Colors::Red );
            if ( threat.splashRange > 0.0f )
            {
              pos.z += 0.1f;
              bot_->debug().drawCircle( pos, threat.splashRange, Colors::Purple );
            }
            pos.z += 0.15f;
            char asd[128];
            sprintf_s( asd, 128, "%.1f dps\r\n%.1f range", threat.dps, threat.range );
            bot_->debug().drawText( asd, pos, Colors::Yellow, 8 );
          }
        }
      }
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
      // the following is old temp junk only left over for WorkerScout to function
      /*if ( enemy.lastSeen_ == 0 )
      {
        // this should be safe, closestLocation could only return false if the map's base location vector is empty
        if ( utils::isMainStructure( msg.unit() ) || bot_->map().closestLocation( msg.unit()->pos )->isStartLocation() )
        {
          bot_->messaging().sendGlobal( M_Intel_FoundPlayer, 2, (size_t)msg.unit()->owner, msg.unit() );
          // TODO separate baseSeen value etc.
          enemy.lastSeen_ = bot_->time();
        }
      }*/
    }
    else if ( msg.code == M_Global_UnitDestroyed && utils::isEnemy( msg.unit() ) )
    {
      if ( utils::isStructure( msg.unit() ) )
      {
        _updateStructure( msg.unit(), true );
      }
      else
      {
        _updateUnit( msg.unit(), true );
      }
    }
  }

  void Intelligence::gameEnd()
  {
    bot_->messaging().remove( this );
  }

}