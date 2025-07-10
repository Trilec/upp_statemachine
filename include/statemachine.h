// StateMachine.h – Asynchronous, event-driven FSM for U++
// -------------------------------------------------------
// Lightweight finite-state-machine helper for orchestrating UI / logic flow in
// Ultimate++ applications.  100% header-only, no external dependencies.
//
// Overview:
//   • Event-based transitions via TriggerEvent()
//   • Async enter/exit using done(bool) callbacks
//   • Transition hooks: Guard / OnBefore / OnAfter
//   • History stack + GoBack()
//
// Example:
//     StateMachine sm;
//     sm.SetInitial("Idle");
//     sm.AddState({ "Idle",       nullptr,          nullptr });
//     sm.AddState({ "Working",    enterWorking,     exitWorking });
//     sm.AddTransition({ "start", "Idle", "Working" });
//     sm.Start();
//     sm.TriggerEvent("start");

#pragma once

#include <Core/Core.h>

namespace Upp {

	// Forward declaration
	class StateMachine;
	
	/// Context passed to Guard / OnBefore / OnAfter callbacks
	struct TransitionContext {
	    StateMachine&      machine;
	    String             fromState;
	    String             toState;
	    String             event;
	
	    TransitionContext(StateMachine& m, String f, String t, String e);
	};
	
	/// A single state with async entry/exit handlers
	struct State {
	    String   id;
	    Function<void(StateMachine&, Function<void(bool)> done)> OnEnter;
	    Function<void(StateMachine&, Function<void(bool)> done)> OnExit;
	};
	
	/// A transition between two states, with optional guard & hooks
	struct Transition {
	    String                                        event;
	    String                                        from;
	    String                                        to;
	    Function<bool(const TransitionContext&)>      Guard;
	    Function<void(const TransitionContext&)>      OnBefore;
	    Function<void(const TransitionContext&)>      OnAfter;
	};
	
	/// Record of a completed transition (for history)
	struct TransitionRecord {
	    String from;
	    String to;
	    String event;
	
	    TransitionRecord(const String& f, const String& t, const String& e)
	      : from(f), to(t), event(e) {}
	};
	
	/// The main FSM class
	class StateMachine {
	public:
	    /// Set the initial state by its ID
	    void SetInitial(const String& id)        { initial = id; }
	
	    /// Add a state definition
	    void AddState(State s);
	
	    /// Add a transition definition
	    void AddTransition(Transition t);
	
	    /// Start the machine in the 'initial' state
	    void Start();
	
	    /// Trigger a named event, causing a transition if defined
	    void TriggerEvent(const String& e);
	
	    /// Attempt the given transition directly
	    bool TryTransition(const Transition& t);
	
	    /// Get current state ID
	    String GetCurrent() const                { return current; }
	
	    /// True if an async transition is in progress
	    bool IsTransitioning() const             { return transitioning; }
	
	    /// True if you can call GoBack()
	    bool CanGoBack() const                   { return transitionHistory.GetCount() > 1; }
	
	    /// Revert to the previous state (if history allows)
	    void GoBack();
	
	    /// Called just before any transition begins
	    Callback1<const TransitionContext&> WhenTransitionStarted;
	
	    /// Called just after any transition completes
	    Callback1<const TransitionContext&> WhenTransitionFinished;
	
	    /// Dump history to LOG()
	    void DumpHistory() const {
	        LOG("StateMachine history:");
	        for(int i = 0; i < transitionHistory.GetCount(); i++) {
	            auto& rec = *transitionHistory[i];
	            LOG(Format("  [%d] %s -> %s (%s)", i, rec.from, rec.to, rec.event));
	        }
	    }
	
	private:
	    const State*       FindState(const String& id) const;
	    const Transition*  FindTransition(const String& from, const String& ev) const;
	
	    void DoTransition(const Transition& t,
	                      bool record = true,
	                      Function<void(bool)> on_done = {});
	
	    void Finalize(const TransitionContext& ctx, bool record);
	
	    Vector< One<State> >            states;
	    Vector< One<Transition> >       transitions;
	    Vector< One<TransitionRecord> > transitionHistory;
	
	    String current;
	    String initial;
	    bool   transitioning = false;
	};

} // namespace Upp
