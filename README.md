# hivemind
Starcraft II AI

Some snippets of code inspired by Dave Churchill's [CommandCenter](https://github.com/davechurchill/CommandCenter) and Blizzard's [s2client-api](https://github.com/Blizzard/s2client-api) examples. Lots of stuff heavily inspired by the BroodWar Terrain Analyzer 2.

Implementation of [linear-time labeling contour-trace](https://www.iis.sinica.edu.tw/papers/fchang/1362-F.pdf) (F.Chang, C-.J-. Cheng, C.-J. Lu 2003) from [BlockoS](https://github.com/BlockoS/blob).
Polygon boolean operations using [ClipperLib](http://www.angusj.com/delphi/clipper.php) by Angus Johnson.
Map hashing using [PicoSHA2](https://github.com/okdshin/PicoSHA2) by okdshin.
Image dumping via [stb_image](https://github.com/nothings/stb/) and [simple_svg](https://code.google.com/archive/p/simple-svg/).


Requirements
* [s2client-api](https://github.com/Blizzard/s2client-api)
* [Boost](http://www.boost.org/)
* [Boost Geometry](https://github.com/boostorg/geometry)
