#include "stdafx.h"
#include "map.h"
#include "map_analysis.h"
#include "utilities.h"
#include "blob_algo.h"
#include "exception.h"
#include "regiongraph.h"

namespace hivemind {

  namespace Analysis {

    nodeID RegionGraph::addNode( const BoostVoronoi::vertex_type* vertex, const Vector2& pos )
    {
      nodeID vID;
      // add new point if not present in the graph
      auto vIt = voronoiVertexToNode.find( vertex );
      if ( vIt == voronoiVertexToNode.end() )
      {
        voronoiVertexToNode[vertex] = vID = nodes.size();
        nodes.push_back( pos );
        nodeType.push_back( NONE );
      } else
      {
        vID = vIt->second;
      }

      return vID;
    }

    nodeID RegionGraph::addNode( const Vector2& pos, const double& minDist )
    {
      nodeID vID;
      // add new point if not present in the graph
      auto it = std::find( nodes.begin(), nodes.end(), pos );
      if ( it == nodes.end() )
      {
        vID = nodes.size();
        nodes.push_back( pos );
        nodeType.push_back( NONE );
        minDistToObstacle.push_back( minDist );
      } else
      {
        vID = it - nodes.begin();
      }

      return vID;
    }

    void RegionGraph::addEdge( const nodeID& v0, const nodeID& v1 )
    {
      // extend adjacency list if needed
      size_t maxId = std::max( v0, v1 );
      if ( adjacencyList.size() < maxId + 1 ) adjacencyList.resize( maxId + 1 );

      adjacencyList[v0].insert( v1 );
      adjacencyList[v1].insert( v0 );
    }

    void RegionGraph::markNodeAsRegion( const nodeID& v0 )
    {
      regionNodes.insert( v0 );
      nodeType[v0] = RegionGraph::REGION;
    }

    void RegionGraph::markNodeAsChoke( const nodeID& v0 )
    {
      chokeNodes.insert( v0 );
      nodeType[v0] = RegionGraph::CHOKEPOINT;
    }

    void RegionGraph::unmarkRegionNode( const nodeID& v0 )
    {
      regionNodes.erase( v0 );
      nodeType[v0] = RegionGraph::NONE;
    }

    void RegionGraph::unmarkChokeNode( const nodeID& v0 )
    {
      chokeNodes.erase( v0 );
      nodeType[v0] = RegionGraph::NONE;
    }

  }

}