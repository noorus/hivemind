#pragma once
#include "hive_types.h"

namespace hivemind {

  namespace AI {

    class Agent;
    class FiniteStateMachine;

    class State {
    protected:
      GameTime mTime;
    public:
      virtual void enter( FiniteStateMachine* machine, Agent* agent ) { mTime = 0; };
      virtual void pause( FiniteStateMachine* machine, Agent* agent ) { leave( machine, agent ); };
      virtual void resume( FiniteStateMachine* machine, Agent* agent ) { enter( machine, agent ); };
      virtual void execute( FiniteStateMachine* machine, Agent* agent, const GameTime delta ) { mTime += delta; };
      virtual void leave( FiniteStateMachine* machine, Agent* agent ) {};
    };

    using StateVector = vector<State*>;

    class FiniteStateMachine {
    protected:
      StateVector mStates;
      Agent* mOwner;
    public:
      FiniteStateMachine( Agent* owner );
      void pushState( State* state );
      void popState();
      void changeState( State* state );
      void execute( const GameTime delta );
    };

  }

}