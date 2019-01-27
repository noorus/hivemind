#pragma once
#include "sc2_forward.h"
#include "hive_math.h"
#include "exception.h"
#include "database.h"

#ifdef HIVE_PLATFORM_WINDOWS
#include "platform_windows.h"
#endif

namespace hivemind {

  struct Vector2;

  //! \class ScopedRWLock
  //! Automation for scoped acquisition and release of an RWLock.
  class ScopedRWLock : boost::noncopyable
  {
  protected:
    platform::RWLock* lock_;
    bool exclusive_;
    bool locked_;

  public:
    //! Constructor.
    //! \param  lock      The lock to acquire.
    //! \param  exclusive (Optional) true to acquire in exclusive mode, false for shared.
    ScopedRWLock( platform::RWLock* lock, bool exclusive = true )
        : lock_( lock ), exclusive_( exclusive ), locked_( true )
    {
      exclusive_ ? lock_->lock() : lock_->lockShared();
    }
    //! Call directly if you want to unlock before object leaves scope.
    void unlock()
    {
      if ( locked_ )
        exclusive_ ? lock_->unlock() : lock_->unlockShared();
      locked_ = false;
    }
    ~ScopedRWLock()
    {
      unlock();
    }
  };

  namespace utils {

    static std::random_device g_randomDevice;
    static std::mt19937 g_random( g_randomDevice() );

    constexpr GameTime timeToTicks( const uint16_t minutes, const uint16_t seconds )
    {
      const float speed_multiplier = 1.4f; // faster
      const GameTime ticks_per_second = 16; // 32 updates per second / 2 updates per tick
      return ( GameTime )( ( ( minutes * 60 ) + seconds ) * ticks_per_second * speed_multiplier );
    }

    inline const RealTime ticksToTime( const GameTime ticks )
    {
      const float speed_multiplier = 1.4f; // faster
      const float ticks_per_second = 16.0f; // 32 updates per second / 2 updates per tick
      return (RealTime)math::round( ( (float)ticks / speed_multiplier ) / ticks_per_second );
    }

    inline int randomBetween( int imin, int imax )
    {
      std::uniform_int_distribution<> distrib( imin, imax );
      return distrib( g_random );
    }

    inline const bool pathable( const GameInfo& info, size_t x, size_t y )
    {
      uint8_t encoded = info.pathing_grid.data[x + ( info.height - 1 - y ) * info.width];
      bool decoded = ( encoded == 255 ? false : true );
      return decoded;
    }

    inline const bool placement( const GameInfo& info, size_t x, size_t y )
    {
      uint8_t encoded = info.placement_grid.data[x + ( info.height - 1 - y ) * info.width];
      bool decoded = ( encoded == 255 ? true : false );
      return decoded;
    }

    inline const Real terrainHeight( const GameInfo& info, size_t x, size_t y )
    {
      uint8_t encoded = info.terrain_height.data[x + ( info.height - 1 - y ) * info.width];
      Real decoded = ( -100.0f + 200.0f * Real( encoded ) / 255.0f );
      return decoded;
    }

    inline const Vector2 calculateCenter( const UnitVector& units )
    {
      Vector2 total( 0.0f, 0.0f );
      if ( units.empty() )
        return total;

      for ( auto unit : units )
      {
        total.x += unit->pos.x;
        total.y += unit->pos.y;
      }

      return ( total / (Real)units.size() );
    }

    inline const bool isMine( const Unit& unit ) { return ( unit.alliance == Unit::Alliance::Self ); }
    inline const bool isMine( const UnitRef unit ) { return ( unit->alliance == Unit::Alliance::Self ); }

    inline const bool isEnemy( const Unit& unit ) { return ( unit.alliance == Unit::Alliance::Enemy ); }
    inline const bool isEnemy( const UnitRef unit ) { return ( unit->alliance == Unit::Alliance::Enemy ); }

    inline const bool isNeutral( const Unit& unit ) { return ( unit.alliance == Unit::Alliance::Neutral ); }
    inline const bool isNeutral( const UnitRef unit ) { return ( unit->alliance == Unit::Alliance::Neutral ); }

    struct isFlying
    {
      inline bool operator()( const Unit& unit )
      {
        return unit.is_flying;
      }
    };

    inline const bool isMainStructure( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::ZERG_HATCHERY:
        case sc2::UNIT_TYPEID::ZERG_LAIR:
        case sc2::UNIT_TYPEID::ZERG_HIVE:
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS:
          return true;
        default:
          return false;
      }
    }

    inline const bool isMainStructure( const Unit& unit ) { return isMainStructure( unit.unit_type ); }
    inline const bool isMainStructure( const UnitRef unit ) { return isMainStructure( unit->unit_type ); }

    struct isMainStructure
    {
      inline bool operator()( const Unit& unit )
      {
        return hivemind::utils::isMainStructure( unit.unit_type );
      }
    };

    inline const bool isRefinery( const UnitTypeID& type )
    {
      return ( Database::unit( type ).resource == UnitData::Resource_GasHarvestable );
    }

    inline Vector2 offsetBuildPosition( UnitTypeID structure, Vector2 position )
    {
      const auto& fp = Database::unit( structure ).footprint;
      return Vector2(
        ( fp.width() % 2 ) ? position.x + 0.5f : position.x,
        ( fp.height() % 2 ) ? position.y + 0.5f : position.y );
    }

    inline const bool isRefinery( const Unit& unit ) { return isRefinery( unit.unit_type ); }
    inline const bool isRefinery( const UnitRef unit ) { return isRefinery( unit->unit_type ); }

    inline const bool isMineral( const UnitTypeID& type )
    {
      return ( Database::unit( type ).resource == UnitData::Resource_MineralsHarvestable );
    }

    inline const bool isMineral( const Unit& unit ) { return isMineral( unit.unit_type ); }
    inline const bool isMineral( const UnitRef unit ) { return isMineral( unit->unit_type ); }

    inline const bool isGeyser( const UnitTypeID& type )
    {
      return ( Database::unit( type ).resource == UnitData::Resource_GasBuildable );
    }

    inline const bool isGeyser( const Unit& unit ) { return isGeyser( unit.unit_type ); }
    inline const bool isGeyser( const UnitRef unit ) { return isGeyser( unit->unit_type ); }

    inline const bool isWorker( const UnitTypeID& type )
    {
      switch ( type.ToType() )
      {
        case sc2::UNIT_TYPEID::TERRAN_SCV:
        case sc2::UNIT_TYPEID::PROTOSS_PROBE:
        case sc2::UNIT_TYPEID::ZERG_DRONE:
        case sc2::UNIT_TYPEID::ZERG_DRONEBURROWED:
          return true;
        default:
          return false;
      }
    }

    inline const bool isWorker( const Unit& unit ) { return isWorker( unit.unit_type ); }
    inline const bool isWorker( const UnitRef unit ) { return isWorker( unit->unit_type ); }

    inline const bool isSupplyProvider( const UnitTypeID& type )
    {
      return ( Database::unit( type ).supplyProvided > 0.0f );
    }

    inline const bool isSupplyProvider( const Unit& unit ) { return isSupplyProvider( unit.unit_type ); }
    inline const bool isSupplyProvider( const UnitRef unit ) { return isSupplyProvider( unit->unit_type ); }

    inline const bool isStructure( const UnitTypeID& type )
    {
      return ( Database::unit( type ).structure );
    }

    inline const bool isStructure( const Unit& unit ) { return isStructure( unit.unit_type ); }
    inline const bool isStructure( const UnitRef unit ) { return isStructure( unit->unit_type ); }

    // This IGNORES collapsibles!
    inline const bool isNeutralBlocker( const UnitRef unit )
    {
      if ( unit->alliance != sc2::Unit::Neutral )
        return false;
      const auto& dbUnit = Database::unit( unit->unit_type );
      return ( dbUnit.structure && !dbUnit.invulnerable && !dbUnit.footprint.empty() && !dbUnit.collapsible );
    }

    inline void hsl2rgb( uint16_t hue, uint8_t sat, uint8_t lum, uint8_t rgb[3] )
    {
      uint16_t r_temp, g_temp, b_temp;
      uint8_t hue_mod;
      uint8_t inverse_sat = ( sat ^ 255 );
      hue = hue % 768;
      hue_mod = hue % 256;
      if ( hue < 256 )
      {
        r_temp = hue_mod ^ 255;
        g_temp = hue_mod;
        b_temp = 0;
      }
      else if ( hue < 512 )
      {
        r_temp = 0;
        g_temp = hue_mod ^ 255;
        b_temp = hue_mod;
      }
      else if ( hue < 768 )
      {
        r_temp = hue_mod;
        g_temp = 0;
        b_temp = hue_mod ^ 255;
      }
      else
      {
        r_temp = 0;
        g_temp = 0;
        b_temp = 0;
      }
      r_temp = ( ( r_temp * sat ) / 255 ) + inverse_sat;
      g_temp = ( ( g_temp * sat ) / 255 ) + inverse_sat;
      b_temp = ( ( b_temp * sat ) / 255 ) + inverse_sat;
      r_temp = ( r_temp * lum ) / 255;
      g_temp = ( g_temp * lum ) / 255;
      b_temp = ( b_temp * lum ) / 255;
      rgb[0] = (uint8_t)r_temp;
      rgb[1] = (uint8_t)g_temp;
      rgb[2] = (uint8_t)b_temp;
    }

    template <class Container, typename Needle, typename DistanceFunction>
    typename Container::value_type findClosestPtr( const Container& haystack, Needle needle, DistanceFunction distanceFunction )
    {
      typename Container::value_type closest{};
      Real bestDistance = std::numeric_limits<Real>::max();
      for ( auto item : haystack )
      {
        Real distance = distanceFunction( item, needle );
        if ( distance < bestDistance || !closest )
        {
          closest = item;
          bestDistance = distance;
        }
      }
      return closest;
    }

    //! Return the unit in set closest to given world position.
    inline const UnitRef findClosestUnit( const std::set<UnitRef>& haystack, const Vector2& pos )
    {
      return findClosestPtr( haystack, pos, []( const Unit* unit, const Vector2& b ) { return b.distance( unit->pos ); } );
    }

    //! Return the unit in vector closest to given world position.
    inline const UnitRef findClosestUnit( const std::vector<UnitRef>& haystack, const Vector2& pos )
    {
      return findClosestPtr( haystack, pos, []( const Unit* unit, const Vector2& b ) { return b.distance( unit->pos ); } );
    }

    inline Real distanceToLineSegment( const Vector2& l0, const Vector2& l1, const Vector2& pt )
    {
      auto diff = ( l1 - l0 );
      auto sqrlen = diff.squaredLength();
      Real t = 0.0f;
      if ( sqrlen )
      {
        t = ( ( pt.x - l0.x ) * diff.x + ( pt.y - l0.y ) * diff.y ) / sqrlen;
        t = math::clamp( t, 0.0f, 1.0f );
      }
      auto ret = ( pt - ( l0 + t * diff ) );
      return ret.length();
    }

    inline Polygon offsetPolygon( const Polygon& poly, Real offset, bool simplify = false )
    {
      auto clipperPoly = util_polyToClipperPath( poly );
      ClipperLib::CleanPolygon( clipperPoly, 1415.0 );
      if ( simplify )
      {
        ClipperLib::Paths out;
        ClipperLib::SimplifyPolygon( clipperPoly, out );
        if ( !out.empty() )
          clipperPoly.swap( out[0] );
      }
      ClipperLib::ClipperOffset co;
      co.AddPath( clipperPoly, ClipperLib::jtSquare, ClipperLib::etClosedPolygon );
      ClipperLib::Paths solution;
      co.Execute( solution, (double)offset * 1000.0f );
      ClipperLib::Path* best = nullptr;
      for ( auto& ret : solution )
      {
        if ( !best || ret.size() > best->size() )
          best = &ret;
      }
      if ( !best )
        return Polygon();
      return util_clipperPathToPolygon( *best );
    }

    inline void readAndHashFile( const string& filename, Sha256& hash )
    {
      platform::FileReader reader( filename );
      auto buffer = malloc( reader.size() );
      reader.read( buffer, (uint32_t)reader.size() );
      auto ptr = (uint8_t*)buffer;
      sha2::sha256_hash( ptr, reader.size(), hash );
      free( buffer );
    }

    inline string bin2hex( const uint8_t* src, size_t length )
    {
      const unsigned char hexchars[] = "0123456789abcdef";

      if ( !length )
        return string();

      string str;
      str.resize( length * 2 );

      for ( size_t i = 0; i < length; i++ )
      {
        str[i * 2 + 0] = hexchars[( src[i] >> 4 ) & 0x0F];
        str[i * 2 + 1] = hexchars[( src[i] ) & 0x0F];
      }

      return std::move( str );
    }

    inline string hexString( const Sha256& hash )
    {
      return bin2hex( hash, 32 );
    }

    inline bool hex2bin( const char* src, uint8_t* target )
    {
      auto char2int = []( char input ) -> int {
        if ( input >= '0' && input <= '9' )
          return input - '0';
        if ( input >= 'A' && input <= 'F' )
          return input - 'A' + 10;
        if ( input >= 'a' && input <= 'f' )
          return input - 'a' + 10;
        return -1;
      };
      while ( *src && src[1] )
      {
        auto first = char2int( *src );
        auto second = char2int( src[1] );
        if ( first == -1 || second == -1 )
          return false;
        *( target++ ) = first * 16 + second;
        src += 2;
      }
      return true;
    }

    template <typename T>
    Color prettyColor( T index, uint16_t seed = 0, uint16_t increment = 120 )
    {
      Color color;
      rgb tmp;
      index += seed;
      utils::hsl2rgb( (uint16_t)index * increment, 255, 250, (uint8_t*)&tmp );
      color.r = tmp.r;
      color.g = tmp.g;
      color.b = tmp.b;
      return color;
    }
  }

  template <class T, class Alloc, class Predicate>
  void erase_if( std::vector<T, Alloc>& c, Predicate pred )
  {
    c.erase( std::remove_if( c.begin(), c.end(), pred ), c.end() );
  }

  template <class Key, class Compare, class Alloc, class Predicate>
  void erase_if( std::set<Key, Compare, Alloc>& c, Predicate pred )
  {
    for ( auto it = c.begin(), last = c.end(); it != last; )
    {
      if ( pred( *it ) )
      {
        it = c.erase( it );
      }
      else
      {
        ++it;
      }
    }
  }

  template<typename T>
  struct span
  {
    span(T* ptr, size_t count):
      ptr_(ptr),
      count_(count)
    {
    }

    span(T* begin, T* end):
      ptr_(begin),
      count_(end - begin)
    {
    }

    T* begin() const
    {
      return ptr_;
    }

    T* end() const
    {
      return ptr_ + count_;
    }

  private:
    T* ptr_;
    size_t count_;
  };
}
