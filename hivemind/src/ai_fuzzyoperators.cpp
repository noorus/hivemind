#include "stdafx.h"
#include "ai_fuzzy.h"

namespace hivemind {

  namespace AI {

    /* FzAND */

    FzAND::FzAND( const FzAND& fa )
    {
      for ( auto& term : fa.terms_ )
        terms_.push_back( term->clone() );
    }

    FzAND::~FzAND()
    {
      for ( auto& term : terms_ )
        SAFE_DELETE( term );
    }

    FzAND::FzAND( FuzzyTerm& op1, FuzzyTerm& op2 )
    {
      terms_.push_back( op1.clone() );
      terms_.push_back( op2.clone() );
    }

    FzAND::FzAND( FuzzyTerm& op1, FuzzyTerm& op2, FuzzyTerm& op3 )
    {
      terms_.push_back( op1.clone() );
      terms_.push_back( op2.clone() );
      terms_.push_back( op3.clone() );
    }

    Real FzAND::dom() const
    {
      Real minimum = std::numeric_limits<Real>::max();

      for ( auto& term : terms_ )
      {
        auto dom = term->dom();
        if ( dom < minimum )
          minimum = dom;
      }

      return minimum;
    }

    void FzAND::clearDOM()
    {
      for ( auto& term : terms_ )
        term->clearDOM();
    }

    void FzAND::ORwithDOM( Real value )
    {
      for ( auto& term : terms_ )
        term->ORwithDOM( value );
    }

    /* FzOR */

    FzOR::FzOR( const FzOR& fa )
    {
      for ( auto& term : fa.terms_ )
        terms_.push_back( term->clone() );
    }

    FzOR::~FzOR()
    {
      for ( auto& term : terms_ )
        SAFE_DELETE( term );
    }

    FzOR::FzOR( FuzzyTerm& op1, FuzzyTerm& op2 )
    {
      terms_.push_back( op1.clone() );
      terms_.push_back( op2.clone() );
    }

    FzOR::FzOR( FuzzyTerm& op1, FuzzyTerm& op2, FuzzyTerm& op3 )
    {
      terms_.push_back( op1.clone() );
      terms_.push_back( op2.clone() );
      terms_.push_back( op3.clone() );
    }

    Real FzOR::dom() const
    {
      Real maximum = std::numeric_limits<Real>::min();

      for ( auto& term : terms_ )
      {
        auto dom = term->dom();
        if ( dom > maximum )
          maximum = dom;
      }

      return maximum;
    }

  }

}