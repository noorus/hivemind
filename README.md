# hivemind
### Starcraft II AI

Some snippets of code inspired by Dave Churchill's [CommandCenter](https://github.com/davechurchill/CommandCenter) and Blizzard's [s2client-api](https://github.com/Blizzard/s2client-api) examples. Lots of stuff *heavily* inspired by the BroodWar Terrain Analyzer 2.

Implementation of [linear-time labeling contour-trace](https://www.iis.sinica.edu.tw/papers/fchang/1362-F.pdf) (F.Chang, C-.J-. Cheng, C.-J. Lu 2003) from [BlockoS](https://github.com/BlockoS/blob).  
Polygon boolean operations using [ClipperLib](http://www.angusj.com/delphi/clipper.php) by Angus Johnson.  
Image dumping via [stb_image](https://github.com/nothings/stb/) and [simple_svg](https://code.google.com/archive/p/simple-svg/).

#### Requirements
* [Boost](http://www.boost.org/)
* [Boost Geometry](https://github.com/boostorg/geometry)
* [s2client-api](https://github.com/Blizzard/s2client-api)

#### Opponents
* [5minBot](https://github.com/Archiatrus/5minBot) (Terran) by Archiatrus
* [CryptBot](https://github.com/Cryptyc/CryptBot) (Protoss) by Cryptyc
* [ByunJR](https://github.com/IanGallacher/ByunJR) (Terran) by Ian Gallacher
* [zerGG](https://github.com/Mirith/zerGG) (Zerg) by Mirith
* [FradBot](https://github.com/DonThompson/FradBot) (Terran) by Don Thompson
* [CommandCenter](https://github.com/davechurchill/commandcenter) (All races) by Dave Churchill