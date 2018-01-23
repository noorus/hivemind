#pragma once

// Whether to support debug-dumping map analysis result images.
#define HIVE_SUPPORT_MAP_DUMPS

#ifdef _RELEASE
# undef HIVE_SUPPORT_MAP_DUMPS
#endif