#pragma once
#include "sc2_forward.h"
#include "hive_types.h"
#include "hive_vector2.h"
#include "hive_vector3.h"
#include "hive_array2.h"
#include "map.h"
#include "map_analysis.h"

namespace hivemind {

  class DebugExtended {
  private:
    sc2::DebugInterface* debug_;
    Point3D mapTileToMarker( const Vector2& v, const Array2<Real>& heightmap, Real offset, Real maxZ );
  public:
    DebugExtended(): debug_( nullptr ) {}
    inline void setForward( sc2::DebugInterface* fwd )
    {
      debug_ = fwd;
    }
    inline sc2::DebugInterface* get() { return debug_; }
    // Currently unimplemented: create unit, kill unit, enemy control, ignore food,
    // give resources, ignore mineral, no cooldowns, all tech, all upgrades, fast build,
    // set energy, set life, set shields, set score
    // ---
    void cheatGodMode()
    {
      debug_->DebugGodMode();
    }
    void cheatIgnoreResourceCost()
    {
      debug_->DebugIgnoreResourceCost();
    }
    void cheatShowMap()
    {
      debug_->DebugShowMap();
    }
    void cheatFastBuild()
    {
      debug_->DebugFastBuild();
    }
    void cheatMoney()
    {
      debug_->DebugGiveAllResources();
    }
    void cheatKillUnit( UnitRef unit )
    {
      debug_->DebugKillUnit( unit );
    }
    void drawText( const string& text, Color color = Colors::White )
    {
      debug_->DebugTextOut( text, color );
    }
    void drawText( const string& text, const Vector2& pos, Color color = Colors::White, uint32_t size = 8 )
    {
      debug_->DebugTextOut( text, pos, color, size );
    }
    void drawText( const string& text, const Vector3& pos, Color color = Colors::White, uint32_t size = 8 )
    {
      debug_->DebugTextOut( text, pos, color, size );
    }
    void drawLine( const Vector3& p0, const Vector3& p1, Color color = Colors::White )
    {
      debug_->DebugLineOut( p0, p1, color );
    }
    void drawBox( const Vector3& p0, const Vector3& p1, Color color = Colors::White )
    {
      debug_->DebugBoxOut( p0, p1, color );
    }
    void endGame( const bool victory = false )
    {
      debug_->DebugEndGame( victory );
    }
    void drawCircle( const Vector3& pos, Real radius, Color color = Colors::White, size_t sides = 16 )
    {
      auto increment = ( (Real)M_PI * 2.0f ) / (Real)sides;
      auto p1 = Vector2::fromAngle( Real( sides - 1 ) * increment, radius ).to3();
      for ( size_t i = 0; i < sides; i++ )
      {
        auto angle = increment * (Real)i;
        auto p2 = Vector2::fromAngle( angle, radius ).to3();
        drawLine( pos + p1, pos + p2, color );
        p1 = p2;
      }
    }
    void drawSphere( const Vector3& p, Real radius, Color color = Colors::White )
    {
      debug_->DebugSphereOut( p, radius, color );
    }
    void moveCamera( const Vector2& pos )
    {
      debug_->DebugMoveCamera( pos );
    }
    void send() { debug_->SendDebug(); }
    void drawMapPolygon( Map& map, Polygon& poly, Color color );
    void drawMapLine( Map& map, const Vector2& p0, const Vector2& p1, Color color );
    void drawMapSphere( Map& map, const Vector2& pos, Real radius, Color color );

  #ifdef HIVE_SUPPORT_MAP_DUMPS
    void mapDumpBasicMaps( Array2<uint64_t>& flagmap, Array2<Real>& heightmap, const GameInfo& info );
    void mapDumpLabelMap( Array2<int>& map, bool contoured, const string& name );
    //! Caution! This is very, VERY slow!
    void mapDumpBaseLocations( Array2<uint64_t>& flagmap, vector<UnitVector>& clusters, const GameInfo& info, BaseLocationVector& bases );
    void mapDumpPolygons( size_t width, size_t height, PolygonComponentVector& polys, Analysis::RegionChokesMap& chokes );
  #else
    inline void mapDumpBasicMaps( Array2<uint64_t>& flagmap, Array2<Real>& heightmap, const GameInfo& info ) { /* disabled */ }
    inline void mapDumpLabelMap( Array2<int>& map, bool contoured, const string& name ) { /* disabled */ }
    inline void mapDumpBaseLocations( Array2<uint64_t>& flagmap, vector<UnitVector>& clusters, const GameInfo& info, BaseLocationVector& bases ) { /* disabled */ }
    inline void mapDumpPolygons( size_t width, size_t height, PolygonComponentVector& polys, Analysis::RegionChokesMap& chokes ) { /* disabled */ }
  #endif
  };

}