#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <set>

namespace hivemind {

  using std::string;
  using std::wstring;
  using std::stringstream;
  using std::vector;
  using std::list;
  using std::set;

  //! \typedef GameTime
  //! \brief Local type definition for game time counted in engine logic ticks.
  //!   (ticks = (seconds * speed_multiplier * ticks_per_second))
  //!   speed_multiplier = 1.4 ("faster" game speed)
  //!   ticks_per_second = 16
  //! \sa RealTime
  using GameTime = uint32_t;

  //! \typedef RealTime
  //! \brief Local type definition for real time counted in seconds.
  //!   (seconds = (ticks / speed_multiplier / ticks_per_second))
  //!   speed_multiplier = 1.4 ("faster" game speed)
  //!   ticks_per_second = 16
  //! \sa GameTime
  using RealTime = uint32_t;

  using PlayerID = uint32_t; //!< A player index number.

  using Real = float; //!< Local floating point type.
  using Radian = Real;

  using StringVector = vector<string>;

  enum CharacterConstants {
    TAB = 9,
    LF = 10,
    CR = 13,
    SPACE = 32,
    QUOTE = 34,
    COLON = 58,
    SEMICOLON = 59,
    BACKSLASH = 92
  };

  using Sha256 = uint8_t[32];

  // This is just a quick nagless conversion from 64-bit pointer to 32-bit uint.
  template <typename T>
  constexpr uint32_t id( T* ptr )
  {
    return (uint32_t)uintptr_t( ptr );
  }

#pragma pack(push, 1)
  struct rgb {
    uint8_t r, g, b;
  };
#pragma pack(pop)

}