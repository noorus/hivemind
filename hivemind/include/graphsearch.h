#pragma once
#include "hive_types.h"
#include "hive_minheap.h"

namespace hivemind {

  namespace graphsearch {

    using SearchableGraphValueType = Real;
    const SearchableGraphValueType c_zeroValue = 0.0f;

    template <class T>
    class SearchableGraph
    {
    public:
      virtual void reset() = 0;
      virtual const bool isValid( T node ) const = 0;
      virtual const bool isClosed( T node ) const = 0;
      virtual set<T> getNeighbours( T node ) = 0;
      virtual SearchableGraphValueType getCost( T from, T to ) = 0;
      virtual void markClosed( T node ) = 0;
      virtual SearchableGraphValueType heuristic( T from, T to ) { return c_zeroValue; }
    };

    template <class T>
    vector<T> genericAStarSearch( SearchableGraph<T>& graph, T start, T goal )
    {
      assert( start != goal );

      Heap<T, SearchableGraphValueType> openTiles;
      std::map<T, T> parent;
      std::map<T, SearchableGraphValueType> gmap;

      openTiles.push( std::make_pair( start, c_zeroValue ) );

      T current;

      while ( !openTiles.empty() )
      {
        current = openTiles.top().first;
        if ( current == goal )
        {
          goto returnCurrent;
        }

        openTiles.pop();
        graph.markClosed( current );

        set<T> neighbours = graph.getNeighbours( current );

        for ( auto neighbour : neighbours )
        {
          if ( !graph.isValid( neighbour ) || graph.isClosed( neighbour ) )
            continue;

          auto g = gmap[current] + graph.getCost( current, neighbour );
          auto h = graph.heuristic( neighbour, goal );
          auto f = g + h;

          if ( gmap.find( neighbour ) == gmap.end() || gmap[neighbour] > g )
          {
            gmap[neighbour] = g;
            openTiles.set( neighbour, f );
            parent[neighbour] = current;
          }
        }
      }

    // unwind from current position to start and return path, regardless of whether we found the goal
    returnCurrent:

      vector<T> reversePath;
      while ( current != parent[current] )
      {
        reversePath.push_back( current );
        current = parent[current];
      }
      reversePath.push_back( start );
      std::reverse( reversePath.begin(), reversePath.end() );
      return reversePath;
    }

  }

}