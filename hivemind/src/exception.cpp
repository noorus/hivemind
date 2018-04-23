#include "stdafx.h"
#include "exception.h"

namespace hivemind {

  Exception::Exception( const string& description ): mDescription( description )
  {
  }

  Exception::Exception( const string& description, const string& source ) :
    mDescription( description ), source_( source )
  {
  }

  const string& Exception::getFullDescription() const
  {
    if ( fullDescription_.empty() )
    {
      stringstream stream;
      stream << mDescription;
      if ( !source_.empty() )
        stream << "\r\nIn function " << source_;
      fullDescription_ = stream.str();
    }

    return fullDescription_;
  }

  const char* Exception::what() const
  {
    return getFullDescription().c_str();
  }

}