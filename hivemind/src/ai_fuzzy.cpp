#include "stdafx.h"
#include "ai_fuzzy.h"

namespace hivemind {

  namespace AI {

    /* FuzzySet */

    FuzzySet::FuzzySet( Real repVal ): dom_( 0.0f ), representativeValue_( repVal )
    {
    }

    void FuzzySet::ORwithDOM( Real value )
    {
      if ( value > dom_ )
        dom_ = value;
    }

    /* FuzzyVariable */

    void FuzzyVariable::adjustRangeToFit( Real min, Real max )
    {
      if ( min < minRange_ )
        minRange_ = min;
      if ( max > maxRange_ )
        maxRange_ = max;
    }

    FuzzyVariable::~FuzzyVariable()
    {
      for ( auto& set : memberSets_ )
        SAFE_DELETE( set.second );
    }

    FzSet FuzzyVariable::addLeftShoulder( const string & name, Real minBound, Real peak, Real maxBound )
    {
      memberSets_[name] = new FuzzySet_LeftShoulder( peak, peak - minBound, maxBound - peak );
      adjustRangeToFit( minBound, maxBound );
      return FzSet( *memberSets_[name] );
    }

    FzSet FuzzyVariable::addRightShoulder( const string & name, Real minBound, Real peak, Real maxBound )
    {
      memberSets_[name] = new FuzzySet_RightShoulder( peak, peak - minBound, maxBound - peak );
      adjustRangeToFit( minBound, maxBound );
      return FzSet( *memberSets_[name] );
    }

    FzSet FuzzyVariable::addTriangular( const string & name, Real minBound, Real peak, Real maxBound )
    {
      memberSets_[name] = new FuzzySet_Triangle( peak, peak - minBound, maxBound - peak );
      adjustRangeToFit( minBound, maxBound );
      return FzSet( *memberSets_[name] );
    }

    FzSet FuzzyVariable::addTrapezoid( const string & name, Real minBound, Real leftPeak, Real rightPeak, Real maxBound )
    {
      memberSets_[name] = new FuzzySet_Trapezoid( minBound, leftPeak, rightPeak, maxBound );
      adjustRangeToFit( minBound, maxBound );
      return FzSet( *memberSets_[name] );
    }

    FzSet FuzzyVariable::addSingleton( const string & name, Real minBound, Real peak, Real maxBound )
    {
      memberSets_[name] = new FuzzySet_Singleton( peak, peak - minBound, maxBound - peak );
      adjustRangeToFit( minBound, maxBound );
      return FzSet( *memberSets_[name] );
    }

    void FuzzyVariable::fuzzify( Real value )
    {
      assert( value >= minRange_ && value <= maxRange_ );

      for ( auto& set : memberSets_ )
      {
        set.second->dom( set.second->calculateDOM( value ) );
      }
    }

    // averaging maxima = sum( maxima * dom ) / sum( doms )
    Real FuzzyVariable::defuzzifyMaxAv() const
    {
      Real top = 0.0f;
      Real bottom = 0.0f;
      for ( auto& set : memberSets_ )
      {
        bottom += set.second->dom();
        top += ( set.second->representativeValue() * set.second->dom() );
      }
      if ( bottom == 0.0f )
        return 0.0f;
      return ( top / bottom );
    }

    Real FuzzyVariable::defuzzifyCentroid( int numSamples ) const
    {
      Real step = ( maxRange_ - minRange_ ) / (Real)numSamples;
      Real totalArea = 0.0f;
      Real sumOfMoments = 0.0f;
      for ( int sample = 1; sample <= numSamples; sample++ )
      {
        for ( auto& set : memberSets_ )
        {
          Real contribution = std::min( set.second->calculateDOM( minRange_ + sample * step ), set.second->dom() );
          totalArea += contribution;
          sumOfMoments += ( minRange_ + sample * step ) * contribution;
        }
      }
      if ( totalArea == 0.0f )
        return 0.0f;
      return ( sumOfMoments / totalArea );
    }

    /* FuzzyModule */

    FuzzyModule::FuzzyModule()
    {
    }

    FuzzyModule::~FuzzyModule()
    {
      for ( auto& var : variables_ )
        SAFE_DELETE( var.second );
      for ( auto& rule : rules_ )
        SAFE_DELETE( rule );
    }

    FuzzyVariable & FuzzyModule::createVariable( const string & name )
    {
      assert( variables_.find( name ) == variables_.end() );

      variables_[name] = new FuzzyVariable( name );
      return *variables_[name];
    }

    void FuzzyModule::addRule( FuzzyTerm & antecedent, FuzzyTerm & consequence )
    {
      rules_.push_back( new FuzzyRule( antecedent, consequence ) );
    }

    void FuzzyModule::fuzzify( const string & name, Real value )
    {
      assert( variables_.find( name ) != variables_.end() );
      variables_[name]->fuzzify( value );
    }

    const int c_numSamples = 15;

    Real FuzzyModule::defuzzify( const string & name, Method method )
    {
      assert( variables_.find( name ) != variables_.end() );
      assert( method == Method_Centroid || method == Method_MaxAV );

      setConfidencesOfConsequentsToZero();

      for ( auto& rule : rules_ )
        rule->calculate();

      if ( method == Method_Centroid )
        return variables_[name]->defuzzifyCentroid( c_numSamples );
      else if ( method == Method_MaxAV )
        return variables_[name]->defuzzifyMaxAv();

      return 0.0f;
    }

    /* FuzzySet_LeftShoulder */

    Real FuzzySet_LeftShoulder::calculateDOM( Real value ) const
    {
      if ( ( rightOffset_ == 0.0f || leftOffset_ == 0.0f ) && peak_ == value )
        return 1.0f;

      if ( value >= peak_ && ( value < ( peak_ + rightOffset_ ) ) )
      {
        Real grad = 1.0f / -rightOffset_;
        return grad * ( value - peak_ ) + 1.0f;
      }
      else if ( value < peak_ && value >= ( peak_ - leftOffset_ ) )
        return 1.0f;
      else
        return 0.0f;
    }

    /* FuzzySet_RightShoulder */

    Real FuzzySet_RightShoulder::calculateDOM( Real value ) const
    {
      if ( ( rightOffset_ == 0.0f || leftOffset_ == 0.0f ) && peak_ == value )
        return 1.0f;

      if ( value <= peak_ && ( value > ( peak_ - leftOffset_ ) ) )
      {
        Real grad = 1.0f / leftOffset_;
        return grad * ( value - ( peak_ - leftOffset_ ) );
      }
      else if ( value > peak_ && ( value <= peak_ + rightOffset_ ) )
        return 1.0f;
      else
        return 0.0f;
    }

    /* FuzzySet_Trapezoid */

    Real FuzzySet_Trapezoid::calculateDOM( Real value ) const
    {
      if ( value < leftBound_ || value > rightBound_ )
        return 0.0f;
      else if ( value >= leftPeak_ && value <= rightPeak_ )
        return 1.0f;

      if ( value < leftPeak_ )
        return ( ( value - leftBound_ ) / ( leftPeak_ - leftBound_ ) );
      else
        return ( 1.0f - ( ( value - rightPeak_ ) / ( rightBound_ - rightPeak_ ) ) );
    }

    /* FuzzySet_Triangle */

    Real FuzzySet_Triangle::calculateDOM( Real value ) const
    {
      if ( ( rightOffset_ == 0.0f || leftOffset_ == 0.0f ) && peak_ == value )
        return 1.0f;

      if ( ( value <= peak_ ) && ( value >= ( peak_ - leftOffset_ ) ) )
      {
        Real grad = 1.0f / leftOffset_;
        return grad * ( value - ( peak_ - leftOffset_ ) );
      }
      else if ( ( value > peak_ ) && ( value < ( peak_ + rightOffset_ ) ) )
      {
        Real grad = 1.0f / -rightOffset_;
        return grad * ( value - peak_ ) + 1.0f;
      }
      else
        return 0.0f;
    }

    /* FuzzySet_Singleton */

    Real FuzzySet_Singleton::calculateDOM( Real value ) const
    {
      if ( value >= ( midPoint_ - leftOffset_ ) && value <= ( midPoint_ + rightOffset_ ) )
        return 1.0f;
      else
        return 0.0f;
    }

  }

}