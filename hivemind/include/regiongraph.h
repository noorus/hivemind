#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"

namespace hivemind {

  namespace Analysis {

    using RegionNodeID = size_t;

    using RegionNodeIDSet = set<RegionNodeID>;

    class RegionGraph {
    public:
      enum NodeType {
        NodeType_None,
        NodeType_Region,
        NodeType_Chokepoint
      };
    private:
      std::map<const BoostVoronoi::vertex_type*, RegionNodeID> voronoiVertexToNode_;
    public:
      // TODO, create struct
      vector<Vector2> nodes;
      vector<RegionNodeIDSet> adjacencyList_;
      vector<double> minDistToObstacle_;
      vector<NodeType> nodeType;

      RegionNodeIDSet regionNodes;
      RegionNodeIDSet chokeNodes;
      RegionNodeID addNode( const BoostVoronoi::vertex_type* vertex, const Vector2& pos );
      RegionNodeID addNode( const Vector2& pos, const double& minDist );
      void addEdge( const RegionNodeID& v0, const RegionNodeID& v1 );
      void markNodeAsRegion( const RegionNodeID& v0 );
      void markNodeAsChoke( const RegionNodeID& v0 );
      void unmarkRegionNode( const RegionNodeID& v0 );
      void unmarkChokeNode( const RegionNodeID& v0 );
    };

  }

}