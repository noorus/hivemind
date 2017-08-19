#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#ifdef _DEBUG
//# define _CRTDBG_MAP_ALLOC
# include <crtdbg.h>
#endif
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <eh.h>
#include <intrin.h>

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

#undef min
#undef max

#pragma warning(push)
#pragma warning(disable: 4251)
#include <sc2api/sc2_api.h>
#include <sc2lib/sc2_lib.h>
#include <sc2utils/sc2_manage_process.h>
#include <sc2renderer/sc2_renderer.h>
#pragma warning(pop)

#pragma warning(disable: 4244)
#include <boost/geometry.hpp>
#pragma warning(default: 4244) 
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/polygon.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <CGAL/basic.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Filtered_kernel.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Segment_Delaunay_graph_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_2.h>
#include <CGAL/Line_2.h>
#include <CGAL/Direction_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_simple_point_location.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Gmpq.h>

#ifndef SAFE_DELETE
# define SAFE_DELETE(p) {if(p){delete p;(p)=NULL;}}
#endif

#ifndef SAFE_CLOSE_HANDLE
# define SAFE_CLOSE_HANDLE(p) {if(p){CloseHandle(p);(p)=NULL;}}
#endif

#include "hive_types.h"