#pragma once

#include "hive_config.h"

#ifdef HIVE_PLATFORM_WINDOWS
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#ifdef HIVE_SUPPORT_GUI
# include <shellapi.h>
# include <shlobj.h>
# include <commctrl.h>
# include <richedit.h>
#endif

#endif // HIVE_PLATFROM_WINDOWS

#ifdef _DEBUG
//# define _CRTDBG_MAP_ALLOC
# include <crtdbg.h>
#endif
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <eh.h>
#include <intrin.h>
#include <assert.h>

#include <exception>
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <strstream>
#include <queue>
#include <regex>
#include <stack>
#include <cstdint>
#include <algorithm>
#include <random>
#include <filesystem>
#include <queue>
#include <unordered_map>
#include <utility>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/find.hpp>

#undef min
#undef max

#pragma warning(push)
#pragma warning(disable: 4251)
#include <sc2api/sc2_api.h>
#include <sc2lib/sc2_lib.h>
#include <sc2utils/sc2_manage_process.h>
#include <sc2renderer/sc2_renderer.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267)
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/polygon.hpp>
#pragma warning(pop)

#include "external/clipper.hpp"
#pragma warning(push)
#pragma warning(disable:4244)
#include "external/picosha2.h"
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4275 4996)
#include <json/json.h>
#pragma warning(pop)

#ifndef SAFE_DELETE
# define SAFE_DELETE(p) {if(p){delete p;(p)=NULL;}}
#endif

#ifndef SAFE_CLOSE_HANDLE
# define SAFE_CLOSE_HANDLE(p) {if(p){CloseHandle(p);(p)=NULL;}}
#endif

#include "hive_types.h"