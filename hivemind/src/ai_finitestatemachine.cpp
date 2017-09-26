#include "stdafx.h"
#include "ai_statemachines.h"

namespace hivemind {

  namespace AI {

    FiniteStateMachine::FiniteStateMachine( Agent* owner ): owner_( owner )
    {
      //
    }

    void FiniteStateMachine::pushState( State* state )
    {
      if ( !states_.empty() )
        states_.back()->pause( this, owner_ );

      states_.push_back( state );

      state->enter( this, owner_ );
    }

    void FiniteStateMachine::popState()
    {
      if ( !states_.empty() )
      {
        states_.back()->leave( this, owner_ );
        states_.pop_back();
      }

      if ( !states_.empty() )
        states_.back()->resume( this, owner_ );
    }

    void FiniteStateMachine::changeState( State* state )
    {
      if ( !states_.empty() )
      {
        states_.back()->leave( this, owner_ );
        states_.pop_back();
      }

      states_.push_back( state );

      state->enter( this, owner_ );
    }

    void FiniteStateMachine::execute( const GameTime delta )
    {
      if ( !states_.empty() )
        states_.back()->execute( this, owner_, delta );
    }

  }

}