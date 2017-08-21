#pragma once
#include "sc2_forward.h"
#include "hive_vector2.h"
#include "hive_array2.h"
#include "hive_polygon.h"
#include "hive_geometry.h"

namespace hivemind {

  namespace Analysis {

    typedef size_t nodeID;

    struct chokeSides_t {
      Vector2 side1;
      Vector2 side2;
      chokeSides_t( Vector2 s1, Vector2 s2 ): side1( s1 ), side2( s2 ) {};
    };

    class RegionGraph {
    private:
      // mapping between Voronoi vertices and graph nodes
      std::map<const BoostVoronoi::vertex_type*, nodeID> voronoiVertexToNode;

    public:
      enum NodeType { NONE, REGION, CHOKEPOINT, CHOKEGATEA, CHOKEGATEB };

      // TODO, create struct
      std::vector<Vector2> nodes;
      std::vector<std::set<nodeID>> adjacencyList;
      std::vector<double> minDistToObstacle;
      std::vector<NodeType> nodeType;

      std::set<nodeID> regionNodes;
      std::set<nodeID> chokeNodes;
      std::set<nodeID> gateNodesA;
      std::set<nodeID> gateNodesB;

      nodeID addNode( const BoostVoronoi::vertex_type* vertex, const Vector2& pos );
      nodeID addNode( const Vector2& pos, const double& minDist );
      void addEdge( const nodeID& v0, const nodeID& v1 );
      void markNodeAsRegion( const nodeID& v0 );
      void markNodeAsChoke( const nodeID& v0 );
      void unmarkRegionNode( const nodeID& v0 );
      void unmarkChokeNode( const nodeID& v0 );
    };

  }

}