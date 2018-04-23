#pragma once

namespace hivemind {

  //! \class Array2
  //! \brief Two-dimensional "dynamic" array class with a templated element type.
  //!   Preferred element access: \c Array2[x][y]
  template<typename T>
  class Array2 {
  private:
    size_t width_;
    size_t height_;
    T* data_;
  public:

    explicit Array2( size_t width = 1, size_t height = 1 ):
      width_( 0 ),
      height_( 0 ),
      data_( nullptr )
    {
      resize( width, height );
    }

    Array2( const Array2& rhs ):
      width_( 0 ),
      height_( 0 ),
      data_( nullptr )
    {
      resize( rhs.width(), rhs.height() );
      memcpy( data_, rhs.data_, width_ * height_ * sizeof( T ) );
    }

    Array2& operator = ( const Array2& rhs )
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
    inline const T* data() const { return data_; }
    inline size_t width() const { return width_; }
    inline size_t height() const { return height_; }

    T* column( size_t n )
    {
      assert(n < width_);

      size_t index = ( n * height_ );
      return &data_[index];
    }

    const T* column( size_t n ) const
    {
      assert(n < width_);

      size_t index = ( n * height_ );
      return &data_[index];
    }

    inline T* operator [] ( size_t n )
    {
      return column( n );
    }

    inline const T* operator [] ( size_t n ) const
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
