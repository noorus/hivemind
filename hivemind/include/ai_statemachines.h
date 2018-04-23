#pragma once
#include "hive_types.h"

namespace hivemind {

  namespace AI {

    class Agent;
    class FiniteStateMachine;

    class State {
    protected:
      GameTime time_;
    public:
      virtual void enter( FiniteStateMachine* machine, Agent* agent ) { time_ = 0; };
      virtual void pause( FiniteStateMachine* machine, Agent* agent ) { leave( machine, agent ); };
      virtual void resume( FiniteStateMachine* machine, Agent* agent ) { enter( machine, agent ); };
      virtual void execute( FiniteStateMachine* machine, Agent* agent, const GameTime delta ) { time_ += delta; };
      virtual void leave( FiniteStateMachine* machine, Agent* agent ) {};
    };

    using StateVector = vector<State*>;

    class FiniteStateMachine {
    protected:
      StateVector states_;
      Agent* owner_;
    public:
      FiniteStateMachine( Agent* owner );
      void pushState( State* state );
      void popState();
      void changeState( State* state );
      void execute( const GameTime delta );
    };

  }

}