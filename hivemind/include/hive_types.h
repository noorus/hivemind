#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <set>

namespace hivemind {

  using std::string;
  using std::stringstream;
  using std::vector;
  using std::list;
  using std::set;

  using GameTime = uint32_t;
  using RealTime = uint32_t;
  using PlayerID = uint32_t;

  using Real = float;
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

}