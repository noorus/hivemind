#pragma once
#include "sc2_forward.h"

namespace hivemind {

  class Bot;

  namespace AI {

    class Agent;

    class Goal {
    public:
      enum Status {
        Status_Active,
        Status_Inactive,
        Status_Completed,
        Status_Failed
      };
    protected:
      Agent* owner_;
      hivemind::Bot* bot_; // hacked in here 'cause it's needed everywhere
      Status status_;
      Real importance_;
      void activateIfInactive();
      void reactivateIfFailed();
    public:
      Goal( Agent* owner );
      virtual ~Goal();
      virtual void activate() = 0;
      virtual Status process() = 0;
      virtual void terminate() = 0;
      void setImportance( const Real importance ) { importance_ = importance; }
      const Real getImportance() const { return importance_; }
      virtual void addSubgoal( Goal* goal );
      virtual const bool isComposite() const { return false; }
      virtual const bool isCollection() const { return false; }
      virtual const string& getName() const = 0;
      const bool isActive() const throw() { return ( status_ == Status_Active ); }
      const bool isInactive() const throw() { return ( status_ == Status_Inactive ); }
      const bool isComplete() const throw() { return ( status_ == Status_Completed ); }
      const bool hasFailed() const throw() { return ( status_ == Status_Failed ); }
      const Status getStatus() const throw() { return status_; }
    };

    using GoalList = std::list<Goal*>;

    class CompositeGoal: public Goal {
    protected:
      GoalList subGoals_;
      Status processSubgoals();
    public:
      CompositeGoal( Agent* owner );
      virtual ~CompositeGoal() override;
      virtual const bool isComposite() const final { return true; }
      virtual const bool isCollection() const final { return false; }
      GoalList& getSubgoals() { return subGoals_; }
      virtual void addSubgoal( Goal* goal ) override;
      virtual void removeAllSubgoals();
    };

    class GoalEvaluator {
    public:
      virtual const Real calculateDesirability( Agent* agent ) = 0;
      virtual void apply( Goal* to, Agent* agent ) = 0;
    };

    using GoalEvaluatorVector = std::vector<GoalEvaluator*>;

    class ThinkGoal: public CompositeGoal {
    protected:
      GoalEvaluatorVector evaluators_;
    public:
      ThinkGoal( Agent* owner );
      void arbitrate();
      void activate() override;
      Status process() override;
      void terminate() override;
    };

    class GoalCollection: public Goal {
    protected:
      GoalList goalList_;
    public:
      GoalCollection( Agent* owner );
      virtual const bool isComposite() const final { return false; }
      virtual const bool isCollection() const final { return true; }
      GoalList& getGoals() { return goalList_; }
      Status process() final;
      virtual void evaluate() = 0;
    };

  }

}