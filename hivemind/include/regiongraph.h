#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"
#include "chokepoint.h"

namespace hivemind {

  namespace Analysis {

    using RegionNodeID = size_t;

    using RegionChokesMap = std::map<RegionNodeID, Chokepoint>;

    class RegionGraph {
    public:
      enum NodeType {
        NodeType_None,
        NodeType_Region,
        NodeType_Chokepoint,
        NodeType_ChokeGateA,
        NodeType_ChokeGateB
      };
    private:
      std::map<const BoostVoronoi::vertex_type*, RegionNodeID> voronoiVertexToNode_;
    public:
      // TODO, create struct
      std::vector<Vector2> nodes;
      std::vector<std::set<RegionNodeID>> adjacencyList;
      std::vector<double> minDistToObstacle;
      std::vector<NodeType> nodeType;

      std::set<RegionNodeID> regionNodes;
      std::set<RegionNodeID> chokeNodes;
      // std::set<RegionNodeID> gateNodesA;
      // std::set<RegionNodeID> gateNodesB;

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