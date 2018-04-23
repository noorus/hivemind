#pragma once
#include "hive_types.h"

namespace hivemind {

  class Exception: public std::exception {
  protected:
    string mDescription;
    string source_;
    mutable string fullDescription_;
  public:
    Exception( const string& description );
    Exception( const string& description, const string& source );
    virtual const std::string& getFullDescription() const;
    virtual const char* what() const throw() override;
  };

#if defined(HIVE_EXCEPT)
# error HIVE_EXCEPT* macro already defined!
#else
# define HIVE_EXCEPT(description) {throw hivemind::Exception(description,__FUNCTION__);}
#endif

}