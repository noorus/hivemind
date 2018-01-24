#pragma once

#define HIVE_PLATFORM_WINDOWS

// Whether to support debug-dumping map analysis result images.
#define HIVE_SUPPORT_MAP_DUMPS

// Whether to enable GUI functionality.
#define HIVE_SUPPORT_GUI

#ifdef _RELEASE
# undef HIVE_SUPPORT_MAP_DUMPS
#endif