#pragma once

namespace hivemind {

  template <class _Tp, class _Val>
  class Heap {
  private:
    std::vector<std::pair< _Tp, _Val >> data;
    std::map< _Tp, int> mapping;
    bool minHeap;
  private:
    int percolate_up( int index )
    {
      if ( index < 0 || index >= (int)data.size() )
        return -1;
      unsigned int parent = ( index - 1 ) / 2;
      int m = ( minHeap ? -1 : 1 );
      while ( index > 0 && m * data[parent].second < m * data[index].second )
      {
        std::pair<_Tp, _Val> temp = data[parent];
        data[parent] = data[index];
        data[index] = temp;
        ( *mapping.find( data[index].first ) ).second = index;
        index = parent;
        parent = ( index - 1 ) / 2;
      }
      ( *mapping.find( data[index].first ) ).second = index;
      return index;
    }
    int percolate_down( int index )
    {
      if ( index < 0 || index >= (int)data.size() )
        return -1;
      auto lchild = index * 2 + 1;
      auto rchild = index * 2 + 2;
      unsigned int mchild;
      int m = ( minHeap ? -1 : 1 );
      while ( ( data.size() > lchild && m*data[index].second < m * data[lchild].second ) ||
        ( data.size() > rchild && m*data[index].second < m * data[rchild].second ) )
      {
        mchild = lchild;
        if ( data.size() > rchild && m * data[rchild].second > m * data[lchild].second )
          mchild = rchild;
        std::pair< _Tp, _Val > temp = data[mchild];
        data[mchild] = data[index];
        data[index] = temp;
        ( *mapping.find( data[index].first ) ).second = index;
        index = mchild;
        lchild = index * 2 + 1;
        rchild = index * 2 + 2;
      }
      ( *mapping.find( data[index].first ) ).second = index;
      return index;
    }
  public:
    Heap( bool isMinHeap = true ): minHeap( isMinHeap ) {}
    ~Heap() {}
    inline bool empty() const { return data.empty(); }
    inline size_t size() const { return data.size(); }
    void push( std::pair< _Tp, _Val > x )
    {
      int index = (int)data.size();
      if ( mapping.insert( std::make_pair( x.first, index ) ).second )
      {
        data.push_back( x );
        percolate_up( index );
      }
    }
    void pop()
    {
      if ( data.empty() )
        return;
      mapping.erase( data.front().first );
      data.front() = data.back();
      data.pop_back();
      if ( data.empty() )
        return;
      std::map<_Tp, int>::iterator iter = mapping.find( data.front().first );
      if ( iter != mapping.end() )
      {
        ( *iter ).second = 0;
        percolate_down( 0 );
      }
    }
    inline const std::pair< _Tp, _Val >& top() const { return data.front(); }
    bool set( _Tp& x, _Val& v )
    {
      std::map<_Tp, int>::iterator iter = mapping.find( x );
      if ( iter == mapping.end() )
      {
        push( std::make_pair( x, v ) );
        return true;
      }
      int index = ( *iter ).second;
      data[index].second = v;
      index = percolate_up( index );
      if ( index >= 0 && index < (int)data.size() )
      {
        percolate_down( index );
        return true;
      }
      return false;
    }
    const _Val& get( _Tp& x ) const
    {
      auto iter = mapping.find( x );
      int index = ( *iter ).second;
      return data[index].second;
    }
    bool contains( _Tp& x ) const
    {
      return ( mapping.find( x ) != mapping.end() );
    }
    void clear()
    {
      data.clear();
      mapping.clear();
    }
    bool erase( _Tp& x )
    {
      auto iter = mapping.find( x );
      if ( iter == mapping.end() )
        return false;
      if ( data.size() == 1 )
        clear();
      else
      {
        int index = ( *iter ).second;
        data[index] = data.back();
        data.pop_back();
        mapping.erase( iter );
        percolate_down( index );
      }
      return true;
    }
  };

}