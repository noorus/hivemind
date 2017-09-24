#pragma once
#include "hive_types.h"
#include "exception.h"

namespace hivemind {

  namespace AI {

    class Agent;
    class FuzzyVariable;
    class FuzzyModule;
    class FuzzyTerm;
    class FzSet;

    using FuzzyVariableMap = std::map<string, FuzzyVariable*>;

    class FuzzyTerm {
    public:
      virtual ~FuzzyTerm() {}
      virtual FuzzyTerm* clone() const = 0;
      virtual Real dom() const = 0;
      virtual void clearDOM() = 0;
      virtual void ORwithDOM( Real value ) = 0;
    };

    using FuzzyTermVector = vector<FuzzyTerm*>;

    class FzAND: public FuzzyTerm {
    private:
      FuzzyTermVector terms_;
      FzAND& operator = ( const FzAND& ) {}
    public:
      virtual ~FzAND() override;
      FzAND( const FzAND& fa );
      FzAND( FuzzyTerm& op1, FuzzyTerm& op2 );
      FzAND( FuzzyTerm& op1, FuzzyTerm& op2, FuzzyTerm& op3 );
      virtual FuzzyTerm* clone() const override { return new FzAND( *this ); }
      virtual Real dom() const override;
      virtual void clearDOM() override;
      virtual void ORwithDOM( Real value ) override;
    };

    class FzOR: public FuzzyTerm {
    private:
      FuzzyTermVector terms_;
      FzOR& operator = ( const FzOR& ) {}
    public:
      virtual ~FzOR() override;
      FzOR( const FzOR& fa );
      FzOR( FuzzyTerm& op1, FuzzyTerm& op2 );
      FzOR( FuzzyTerm& op1, FuzzyTerm& op2, FuzzyTerm& op3 );
      virtual FuzzyTerm* clone() const override { return new FzOR( *this ); }
      virtual Real dom() const override;
      virtual void clearDOM() override { HIVE_EXCEPT( "Invalid context" ); }
      virtual void ORwithDOM( Real value ) override { HIVE_EXCEPT( "Invalid context" ); }
    };

    class FuzzyRule {
    private:
      const FuzzyTerm* antecedent_;
      FuzzyTerm* consequence_;
      FuzzyRule( const FuzzyRule& ) {}
      FuzzyRule& operator = ( const FuzzyRule& ) {}
    public:
      FuzzyRule( const FuzzyTerm& antecedent, const FuzzyTerm& consequence ):
        antecedent_( antecedent.clone() ), consequence_( consequence.clone() ) {}
      ~FuzzyRule() {
        SAFE_DELETE( antecedent_ );
        SAFE_DELETE( consequence_ );
      }
      inline void setConfidenceOfConsequentToZero() { consequence_->clearDOM(); }
      inline void calculate() { consequence_->ORwithDOM( antecedent_->dom() ); }
    };

    using FuzzyRuleVector = vector<FuzzyRule*>;

    class FuzzySet {
    protected:
      Real dom_;
      Real representativeValue_;
    public:
      FuzzySet( Real repVal );
      virtual Real calculateDOM( Real value ) const = 0;
      void ORwithDOM( Real value );
      Real representativeValue() const { return representativeValue_; }
      void clearDOM() { dom_ = 0.0f; }
      inline Real dom() const { return dom_; }
      inline void dom( Real value ) { dom_ = value; }
    };

    using FuzzySetMap = std::map<string, FuzzySet*>;

    class FuzzySet_LeftShoulder: public FuzzySet {
    private:
      Real peak_;
      Real leftOffset_;
      Real rightOffset_;
    public:
      FuzzySet_LeftShoulder( Real peak, Real left, Real right ):
        FuzzySet( ( ( peak - left ) + peak ) / 2 ), peak_( peak ), leftOffset_( left ), rightOffset_( right ) {}
      virtual Real calculateDOM( Real value ) const override;
    };

    class FuzzySet_Triangle: public FuzzySet {
    private:
      Real peak_;
      Real leftOffset_;
      Real rightOffset_;
    public:
      FuzzySet_Triangle( Real mid, Real left, Real right ):
        FuzzySet( mid ), peak_( mid ), leftOffset_( left ), rightOffset_( right ) {}
      virtual Real calculateDOM( Real value ) const override;
    };

    class FuzzySet_Trapezoid: public FuzzySet {
    private:
      Real leftBound_;
      Real leftPeak_;
      Real rightPeak_;
      Real rightBound_;
    public:
      FuzzySet_Trapezoid( Real left, Real leftMid, Real rightMid, Real right ):
        FuzzySet( leftMid ), leftBound_( left ), leftPeak_( leftMid ), rightPeak_( rightMid ), rightBound_( right ) {}
      virtual Real calculateDOM( Real value ) const override;
    };

    class FuzzySet_RightShoulder: public FuzzySet {
    private:
      Real peak_;
      Real leftOffset_;
      Real rightOffset_;
    public:
      FuzzySet_RightShoulder( Real peak, Real left, Real right ):
        FuzzySet( ( ( peak + right ) + peak ) / 2 ), peak_( peak ), leftOffset_( left ), rightOffset_( right ) {}
      virtual Real calculateDOM( Real value ) const override;
    };

    class FuzzySet_Singleton: public FuzzySet {
    private:
      Real midPoint_;
      Real leftOffset_;
      Real rightOffset_;
    public:
      FuzzySet_Singleton( Real mid, Real left, Real right ):
        FuzzySet( mid ), midPoint_( mid ), leftOffset_( left ), rightOffset_( right ) {}
      virtual Real calculateDOM( Real value ) const override;
    };

    class FuzzyVariable {
      friend class FuzzyModule;
    private:
      string name_;
      FuzzySetMap memberSets_;
      Real minRange_;
      Real maxRange_;
      void adjustRangeToFit( Real min, Real max );
      ~FuzzyVariable();
    public:
      explicit FuzzyVariable( const string& name ): minRange_( 0.0f ), maxRange_( 0.0f ), name_( name ) {}
      inline const FuzzySetMap& members() const { return memberSets_; }
      inline const string& name() const { return name_; }
      FzSet addLeftShoulder( const string& name, Real minBound, Real peak, Real maxBound );
      FzSet addRightShoulder( const string& name, Real minBound, Real peak, Real maxBound );
      FzSet addTriangular( const string& name, Real minBound, Real peak, Real maxBound );
      FzSet addTrapezoid( const string& name, Real minBound, Real leftPeak, Real rightPeak, Real maxBound );
      FzSet addSingleton( const string& name, Real minBound, Real peak, Real maxBound );
      void fuzzify( Real value );
      Real defuzzifyMaxAv() const;
      Real defuzzifyCentroid( int numSamples ) const;
    };

    class FzSet: public FuzzyTerm {
      friend class FzVery;
      friend class FzFairly;
    private:
      FuzzySet& set_;
    public:
      FzSet( FuzzySet& set ): set_( set ) {}
      virtual FuzzyTerm* clone() const override { return new FzSet( *this ); }
      virtual Real dom() const override { return set_.dom(); }
      virtual void clearDOM() override { set_.clearDOM(); }
      virtual void ORwithDOM( Real value ) override { set_.ORwithDOM( value ); }
    };

    class FuzzyModule {
    public:
      enum Method {
        Method_MaxAV,
        Method_Centroid
      };
    private:
      FuzzyVariableMap variables_;
      FuzzyRuleVector rules_;
      inline void setConfidencesOfConsequentsToZero()
      {
        for ( auto& rule : rules_ )
          rule->setConfidenceOfConsequentToZero();
      }
    public:
      FuzzyModule();
      ~FuzzyModule();
      FuzzyVariable& createVariable( const string& name );
      void addRule( FuzzyTerm& antecedent, FuzzyTerm& consequence );
      void fuzzify( const string& name, Real value );
      Real defuzzify( const string& key, Method method = Method_MaxAV );
    };

  }

}