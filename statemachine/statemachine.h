// statemachine/statemachine.h
// Asynchronous, event-driven FSM for U++
// -------------------------------------------------------
// Lightweight finite-state-machine helper for orchestrating UI / logic flow in
// Ultimate++ applications. No external dependencies.
//
// Overview:
//   • Event-based transitions via TriggerEvent()
//   • Async enter/exit using done(bool) callbacks
//   • Transition hooks: Guard / OnBefore / OnAfter
//   • History stack + GoBack()
//
// Example:
//     StateMachine sm;
//     if(!sm.AddState({ "Idle", {}, {} }))
//         return;
//     if(!sm.AddState({ "Working", enterWorking, exitWorking }))
//         return;
//     if(!sm.SetInitial("Idle"))
//         return;
//     if(!sm.AddTransition({ "start", "Idle", "Working" }))
//         return;
//     if(!sm.Start())
//         LOG(sm.GetLastErrorText());
//     if(!sm.TriggerEvent("start"))
//         LOG(sm.GetLastErrorText());

#pragma once

#include <Core/Core.h>

namespace Upp {

	enum class StateMachineError {
		None,
		EmptyStateId,
		DuplicateStateId,
		EmptyEvent,
		EmptyFromState,
		EmptyToState,
		MissingState,
		MissingFromState,
		MissingToState,
		DuplicateTransition,
		AlreadyStarted,
		NotStarted,
		TransitionInProgress,
		NoMatchingTransition,
		GuardRejected,
		WrongSourceState,
		StartEnterFailed,
		ExitFailed,
		EnterFailed,
		BackTransitionFailed,
		EventRejectedWhileTransitioning,
		EventDroppedWhileTransitioning,
		EventQueueFull,
		EventQueueingNotImplemented,
	};

	// Only TriggerEvent() participates in queueing under QueueWhileTransitioning.
	enum class EventPolicy {
		RejectWhileTransitioning,
		DropWhileTransitioning,
		QueueWhileTransitioning,
	};

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
	    bool SetInitial(const String& id) {
	        if (id.IsEmpty()) {
	            last_error = StateMachineError::EmptyStateId;
	            return false;
	        }
	        if (started) {
	            last_error = StateMachineError::AlreadyStarted;
	            return false;
	        }
	        initial = id;
	        ClearError();
	        return true;
	    }

	    /// Get the configured initial state ID
	    String GetInitial() const                { return initial; }

	    /// True if an initial state has been configured
	    bool HasInitial() const                  { return !initial.IsEmpty(); }
	
	    /// Add a state definition. Returns false for invalid or late additions.
	    bool AddState(State s);
	
	    /// Add a transition definition. Returns false for invalid or late additions.
	    bool AddTransition(Transition t);

	    /// Check whether a state exists.
	    bool HasState(const String& id) const;

	    /// Check whether a transition exists.
	    bool HasTransition(const String& from, const String& event) const;

	    /// Get the number of configured states.
	    int GetStateCount() const;

	    /// Get the number of configured transitions.
	    int GetTransitionCount() const;
	
	    /// Start the machine in the 'initial' state
	    bool Start();
	
	    /// Trigger a named event, causing a transition if defined
	    bool TriggerEvent(const String& e);

	    /// Attempt the given transition directly
	    bool TryTransition(const Transition& t);

	    /// Enable or disable internal logging
	    void EnableLogging(bool b = true)   { logging = b; }

	    /// True if internal logging is enabled
	    bool IsLoggingEnabled() const       { return logging; }

	    /// Set the event policy used while a transition is already in progress
	    void SetEventPolicy(EventPolicy policy) { event_policy = policy; ClearError(); }

	    /// Get the stored event policy
	    EventPolicy GetEventPolicy() const { return event_policy; }

	    /// Configure the bounded queued-event capacity used by QueueWhileTransitioning.
	    void SetMaxQueuedEvents(int n);
	    int GetMaxQueuedEvents() const { return max_queued_events; }

	    /// Queue inspection and control for pending event names.
	    int GetQueuedEventCount() const { return queued_events.GetCount(); }
	    bool HasQueuedEvents() const { return !queued_events.IsEmpty(); }
	    void ClearQueuedEvents() { queued_events.Clear(); ClearError(); }
	
	    /// Get current state ID
	    String GetCurrent() const                { return current; }

	    /// True if Start() has been accepted and the machine owns a current initial state
	    bool IsStarted() const                   { return started; }
	
	    /// True if an async transition is in progress
	    bool IsTransitioning() const             { return transitioning; }
	
	    /// True if you can call GoBack()
	    bool CanGoBack() const                   { return transitionHistory.GetCount() > 1; }

	    /// Current error code from the last failing public call
	    StateMachineError GetLastError() const    { return last_error; }

	    /// Stable human-readable description of the last error
	    String GetLastErrorText() const;

	    /// Clear the last error
	    void ClearError()                        { last_error = StateMachineError::None; }

	    /// History inspection for tests and diagnostics
	    int GetHistoryCount() const              { return transitionHistory.GetCount(); }
	    String GetHistoryFrom(int i) const       { return (i >= 0 && i < transitionHistory.GetCount()) ? transitionHistory[i]->from : String(); }
	    String GetHistoryTo(int i) const         { return (i >= 0 && i < transitionHistory.GetCount()) ? transitionHistory[i]->to : String(); }
	    String GetHistoryEvent(int i) const      { return (i >= 0 && i < transitionHistory.GetCount()) ? transitionHistory[i]->event : String(); }
	
	    /// Revert to the previous state (if history allows)
	    bool GoBack();

	    /// Reset runtime state while keeping configuration
	    bool Reset();

	    /// Clear all runtime state and configuration
	    bool Clear();
	
	    /// Called just before any transition begins
	    Function<void(const TransitionContext&)> WhenTransitionStarted;
	
	    /// Called just after any transition completes
	    Function<void(const TransitionContext&)> WhenTransitionFinished;
	
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
	
	    bool DoTransition(const Transition& t,
	                      bool record = true,
	                      Function<void(bool)> on_done = {});
	    bool QueueEvent(const String& e);
	    void DrainQueuedEvents();
	
	    void Finalize(const TransitionContext& ctx, bool record);
	
	    Vector< One<State> >            states;
	    Vector< One<Transition> >       transitions;
	    Vector< One<TransitionRecord> > transitionHistory;

	    String current;
	    String initial;
	    bool   started = false;
	    bool   transitioning = false;
	    bool   logging = false;
	    bool   processing_queue = false;
	    EventPolicy event_policy = EventPolicy::RejectWhileTransitioning;
	    Vector<String> queued_events;
	    int max_queued_events = 64;
	    StateMachineError last_error = StateMachineError::None;
	};

} // namespace Upp
