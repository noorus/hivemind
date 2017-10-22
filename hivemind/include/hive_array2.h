#pragma once
#include "hive_types.h"
#include "sc2_forward.h"

namespace hivemind {

  //< Use accessors: Array2[x][y]
  template<typename T>
  class Array2 {
  private:
    size_t width_, height_;
    T* data_;
  public:
    Array2( size_t width = 1, size_t height = 1, T* data = nullptr ):
    width_( 0 ), height_( 0 ), data_( nullptr )
    {
      resize( width, height );
    }
    Array2( const Array2<T>& rhs ):
    width_( 0 ), height_( 0 ), data_( nullptr )
    {
      resize( rhs.width(), rhs.height() );
      memcpy( data_, rhs.data_, width_ * height_ * sizeof( T ) );
    }
    const Array2& operator = ( const Array2<T>& rhs )
    {
      resize( rhs.width(), rhs.height() );
      memcpy( data_, rhs.data_, width_ * height_ * sizeof( T ) );
      return *this;
    }
    ~Array2()
    {
      if ( data_ )
        free( data_ );
    }
    const bool empty() const { return ( width_ == 0 && height_ == 0 ); }
    inline T* data() { return data_; }
    inline size_t width() const { return width_; }
    inline size_t height() const { return height_; }
    T* column( size_t n )
    {
      size_t index = ( n * width_ );
      return &data_[index];
    }
    T* column( size_t n ) const
    {
      size_t index = ( n * width_ );
      return &data_[index];
    }
    inline T* operator [] ( size_t n )
    {
      return column( n );
    }
    inline T* operator [] ( size_t n ) const
    {
      return column( n );
    }
    void resize( size_t width, size_t height )
    {
      width_ = width;
      height_ = height;

      if ( !data_ )
        data_ = (T*)malloc( width_ * height_ * sizeof( T ) );
      else
        data_ = (T*)realloc( data_, width_ * height_ * sizeof( T ) );
    }
    void reset( T value )
    {
      if ( !data_ )
        return;
      for ( size_t i = 0; i < ( width_ * height_ ); i++ )
        data_[i] = value;
    }
  };

}