#include "stdafx.h"
#include "map.h"
#include "map_analysis.h"
#include "utilities.h"
#include "blob_algo.h"
#include "exception.h"
#include "regiongraph.h"

namespace hivemind {

  namespace Analysis {

    RegionNodeID RegionGraph::addNode( const BoostVoronoi::vertex_type* vertex, const Vector2& pos )
    {
      RegionNodeID vID;

      auto vIt = voronoiVertexToNode_.find( vertex );
      if ( vIt == voronoiVertexToNode_.end() )
      {
        voronoiVertexToNode_[vertex] = vID = nodes.size();
        nodes.push_back( pos );
        nodeType.push_back( NodeType_None );
      }
      else
        vID = vIt->second;

      return vID;
    }

    RegionNodeID RegionGraph::addNode( const Vector2& pos, const double& minDist )
    {
      RegionNodeID vID;

      auto it = std::find( nodes.begin(), nodes.end(), pos );
      if ( it == nodes.end() )
      {
        vID = nodes.size();
        nodes.push_back( pos );
        nodeType.push_back( NodeType_None );
        minDistToObstacle_.push_back( minDist );
      } else
        vID = it - nodes.begin();

      return vID;
    }

    void RegionGraph::addEdge( const RegionNodeID& v0, const RegionNodeID& v1 )
    {
      size_t maxId = std::max( v0, v1 );

      if ( adjacencyList_.size() < maxId + 1 )
        adjacencyList_.resize( maxId + 1 );

      adjacencyList_[v0].insert( v1 );
      adjacencyList_[v1].insert( v0 );
    }

    void RegionGraph::markNodeAsRegion( const RegionNodeID& v0 )
    {
      regionNodes.insert( v0 );
      nodeType[v0] = RegionGraph::NodeType_Region;
    }

    void RegionGraph::markNodeAsChoke( const RegionNodeID& v0 )
    {
      chokeNodes.insert( v0 );
      nodeType[v0] = RegionGraph::NodeType_Chokepoint;
    }

    void RegionGraph::unmarkRegionNode( const RegionNodeID& v0 )
    {
      regionNodes.erase( v0 );
      nodeType[v0] = RegionGraph::NodeType_None;
    }

    void RegionGraph::unmarkChokeNode( const RegionNodeID& v0 )
    {
      chokeNodes.erase( v0 );
      nodeType[v0] = RegionGraph::NodeType_None;
    }

  }

}