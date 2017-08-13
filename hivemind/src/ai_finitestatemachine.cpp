#include "stdafx.h"
#include "ai_statemachines.h"

namespace hivemind {

  namespace AI {

    FiniteStateMachine::FiniteStateMachine( Agent* owner ): mOwner( owner )
    {
      //
    }

    void FiniteStateMachine::pushState( State* state )
    {
      if ( !mStates.empty() )
        mStates.back()->pause( this, mOwner );

      mStates.push_back( state );

      state->enter( this, mOwner );
    }

    void FiniteStateMachine::popState()
    {
      if ( !mStates.empty() )
      {
        mStates.back()->leave( this, mOwner );
        mStates.pop_back();
      }

      if ( !mStates.empty() )
        mStates.back()->resume( this, mOwner );
    }

    void FiniteStateMachine::changeState( State* state )
    {
      if ( !mStates.empty() )
      {
        mStates.back()->leave( this, mOwner );
        mStates.pop_back();
      }

      mStates.push_back( state );

      state->enter( this, mOwner );
    }

    void FiniteStateMachine::execute( const GameTime delta )
    {
      if ( !mStates.empty() )
        mStates.back()->execute( this, mOwner, delta );
    }

  }

}