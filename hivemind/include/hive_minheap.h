#pragma once

namespace hivemind {

  template <typename IndexType, class ValueType>
  class Heap {
  private:
    vector<std::pair<IndexType, ValueType>> data_;
    std::map<IndexType, int> mapping_;
    bool minHeap_;
  private:
    int percolate_up( int index )
    {
      if ( index < 0 || index >= (int)data_.size() )
        return -1;

      unsigned int parent = ( index - 1 ) / 2;

      int m = ( minHeap_ ? -1 : 1 );

      while ( index > 0 && m * data_[parent].second < m * data_[index].second )
      {
        auto temp = data_[parent];
        data_[parent] = data_[index];
        data_[index] = temp;
        ( *mapping_.find( data_[index].first ) ).second = index;
        index = parent;
        parent = ( index - 1 ) / 2;
      }

      ( *mapping_.find( data_[index].first ) ).second = index;

      return index;
    }
    int percolate_down( int index )
    {
      if ( index < 0 || index >= (int)data_.size() )
        return -1;

      auto lchild = index * 2 + 1;
      auto rchild = index * 2 + 2;
      unsigned int mchild;

      int m = ( minHeap_ ? -1 : 1 );

      while ( ( data_.size() > lchild && m*data_[index].second < m * data_[lchild].second )
        || ( data_.size() > rchild && m*data_[index].second < m * data_[rchild].second ) )
      {
        mchild = lchild;
        if ( data_.size() > rchild && m * data_[rchild].second > m * data_[lchild].second )
          mchild = rchild;

        auto temp = data_[mchild];
        data_[mchild] = data_[index];
        data_[index] = temp;

        ( *mapping_.find( data_[index].first ) ).second = index;

        index = mchild;

        lchild = index * 2 + 1;
        rchild = index * 2 + 2;
      }

      ( *mapping_.find( data_[index].first ) ).second = index;

      return index;
    }
  public:
    Heap( bool minheap = true ): minHeap_( minheap ) {}
    ~Heap() {}
    inline bool empty() const { return data_.empty(); }
    inline size_t size() const { return data_.size(); }
    inline const std::pair<IndexType, ValueType>& top() const { return data_.front(); }
    void push( std::pair< IndexType, ValueType > x )
    {
      int index = (int)data_.size();

      if ( mapping_.insert( std::make_pair( x.first, index ) ).second )
      {
        data_.push_back( x );
        percolate_up( index );
      }
    }
    void pop()
    {
      if ( data_.empty() )
        return;

      mapping_.erase( data_.front().first );
      data_.front() = data_.back();
      data_.pop_back();

      if ( data_.empty() )
        return;

      auto iter = mapping_.find( data_.front().first );
      if ( iter != mapping_.end() )
      {
        ( *iter ).second = 0;
        percolate_down( 0 );
      }
    }
    bool set( IndexType& x, ValueType& v )
    {
      auto iter = mapping_.find( x );
      if ( iter == mapping_.end() )
      {
        push( std::make_pair( x, v ) );
        return true;
      }

      int index = ( *iter ).second;
      data_[index].second = v;

      index = percolate_up( index );

      if ( index >= 0 && index < (int)data_.size() )
      {
        percolate_down( index );
        return true;
      }
      return false;
    }
    const ValueType& get( IndexType& x ) const
    {
      auto iter = mapping_.find( x );
      int index = ( *iter ).second;
      return data_[index].second;
    }
    bool contains( IndexType& x ) const
    {
      return ( mapping_.find( x ) != mapping_.end() );
    }
    void clear()
    {
      data_.clear();
      mapping_.clear();
    }
    bool erase( IndexType& x )
    {
      auto iter = mapping_.find( x );
      if ( iter == mapping_.end() )
        return false;

      if ( data_.size() == 1 )
        clear();
      else
      {
        int index = ( *iter ).second;
        data_[index] = data_.back();
        data_.pop_back();
        mapping_.erase( iter );
        percolate_down( index );
      }

      return true;
    }
  };

}