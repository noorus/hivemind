#pragma once

namespace hivemind {


  struct InplaceHeapNode
  {
    explicit InplaceHeapNode() :
      index(-1)
    {
    }

    int index;
  };

  template <typename NodeType, typename ValueType>
  class InplaceHeap {
  private:

    static_assert(std::is_base_of_v<InplaceHeapNode, NodeType>, "The NodeType for InplaceHeap must be derived from InplaceHeapNode");

    using IndexType = NodeType*;

    vector<std::pair<IndexType, ValueType>> data_;

  private:

    int percolate_up( int index )
    {
      if ( index < 0 || index >= (int)data_.size() )
        return -1;

      unsigned int parent = ( index - 1 ) / 2;

      while ( index > 0 && data_[parent].second > data_[index].second )
      {
        auto temp = data_[parent];
        data_[parent] = data_[index];
        data_[index] = temp;
        data_[index].first->index = index;
        index = parent;
        parent = ( index - 1 ) / 2;
      }

      data_[index].first->index = index;

      return index;
    }

    int percolate_down( int index )
    {
      if ( index < 0 || index >= (int)data_.size() )
        return -1;

      auto lchild = index * 2 + 1;
      auto rchild = index * 2 + 2;
      unsigned int mchild;

      while ( ( data_.size() > lchild && data_[index].second > data_[lchild].second )
        || ( data_.size() > rchild && data_[index].second > data_[rchild].second ) )
      {
        mchild = lchild;
        if ( data_.size() > rchild && data_[rchild].second < data_[lchild].second )
          mchild = rchild;

        auto temp = data_[mchild];
        data_[mchild] = data_[index];
        data_[index] = temp;
        data_[index].first->index = index;

        index = mchild;

        lchild = index * 2 + 1;
        rchild = index * 2 + 2;
      }

      data_[index].first->index = index;

      return index;
    }
  public:
    InplaceHeap() {}
    ~InplaceHeap() {}

    inline bool empty() const { return data_.empty(); }
    inline size_t size() const { return data_.size(); }

    inline std::pair<IndexType, ValueType>& top()
    {
      assert(!empty());
      return data_.front();
    }

    inline const std::pair<IndexType, ValueType>& top() const
    {
      assert(!empty());
      return data_.front();
    }

    inline const ValueType topKey() const
    {
      assert(!empty());
      return data_.front().second;
    }

    void push( std::pair< IndexType, ValueType > x )
    {
      int index = (int)data_.size();

      assert(x.first->index == -1);
      x.first->index = index;
      data_.push_back( x );

      percolate_up( index );
    }

    void pop()
    {
      if ( data_.empty() )
        return;

      data_.back().first->index = 0;
      data_.front().first->index = -1;
      data_.front() = data_.back();
      data_.pop_back();

      if ( data_.empty() )
        return;

      percolate_down( 0 );
    }

    bool set( IndexType x, ValueType v )
    {
      if(x->index == -1)
      {
        push( std::make_pair( x, v ) );
        return true;
      }

      int index = x->index;
      data_[index].second = v;

      index = percolate_up( index );

      if ( index >= 0 && index < (int)data_.size() )
      {
        percolate_down( index );
        return true;
      }
      return false;
    }

    void clear()
    {
      data_.clear();
    }

    bool erase( IndexType x )
    {
      if(x->index == -1)
        return false;

      assert(x->index < data_.size());

      if ( data_.size() == 1 )
        clear();
      else
      {
        int index = x->index;
        data_.back().first->index = index;
        data_[index].first->index = -1;
        data_[index] = data_.back();
        data_.pop_back();
        percolate_down( index );
      }

      return true;
    }
  };

}