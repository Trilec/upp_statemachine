/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    StateMachine implementation
    ===========================

    Purpose
    - Implements the compact U++ StateMachine core declared in
      statemachine/statemachine.h.

    Intent
    - Keep transition execution deterministic and easy to audit.
    - Preserve source state until OnEnter succeeds.
    - Commit current/history before WhenTransitionFinished and OnAfter.
    - Keep failure rollback explicit: failed exit/enter/startup must not corrupt
      current state or history.
    - Keep QueueWhileTransitioning lightweight: bounded FIFO event-name queue,
      drained only after successful completion.

    Thread context
    - Same-thread / same-callback-chain use.
    - No internal locking or background execution.

    Usage
    - Include statemachine/statemachine.h from client code.
    - See README.md and docs/API.md for public examples and contract details.

    Changelog
    - 2026-06: v1.0.1 release-prep cleanup after queueing, async rollback,
      and invariant-test hardening.
*/
#include "statemachine.h"

#include <memory>

namespace Upp {

static String GetStateMachineErrorText(StateMachineError error) {
    switch(error) {
    case StateMachineError::None: return "None";
    case StateMachineError::EmptyStateId: return "Empty state id";
    case StateMachineError::DuplicateStateId: return "Duplicate state id";
    case StateMachineError::EmptyEvent: return "Empty event";
    case StateMachineError::EmptyFromState: return "Empty from state";
    case StateMachineError::EmptyToState: return "Empty to state";
    case StateMachineError::MissingState: return "Missing state";
    case StateMachineError::MissingFromState: return "Missing from state";
    case StateMachineError::MissingToState: return "Missing to state";
    case StateMachineError::DuplicateTransition: return "Duplicate transition";
    case StateMachineError::AlreadyStarted: return "Already started";
    case StateMachineError::NotStarted: return "Not started";
    case StateMachineError::TransitionInProgress: return "Transition in progress";
    case StateMachineError::NoMatchingTransition: return "No matching transition";
    case StateMachineError::GuardRejected: return "Guard rejected";
    case StateMachineError::WrongSourceState: return "Wrong source state";
    case StateMachineError::StartEnterFailed: return "Start enter failed";
    case StateMachineError::ExitFailed: return "Exit failed";
    case StateMachineError::EnterFailed: return "Enter failed";
    case StateMachineError::BackTransitionFailed: return "Back transition failed";
    case StateMachineError::EventRejectedWhileTransitioning: return "Event rejected while transitioning";
    case StateMachineError::EventDroppedWhileTransitioning: return "Event dropped while transitioning";
    case StateMachineError::EventQueueFull: return "Event queue full";
    case StateMachineError::EventQueueDrainLimitReached: return "Event queue drain limit reached";
    }
    return "Unknown error";
}

String StateMachine::GetLastErrorText() const {
    return GetStateMachineErrorText(last_error);
}

//------------------------------------------------------------------------------
// TransitionContext carries context during a transition
//------------------------------------------------------------------------------
TransitionContext::TransitionContext(StateMachine& m, String f, String t, String e)
    : machine(m)
    , fromState(pick(f))
    , toState(pick(t))
    , event(pick(e))
{}

//------------------------------------------------------------------------------
// Add a new state definition
//------------------------------------------------------------------------------
bool StateMachine::AddState(State s) {
    if (started) {
        last_error = StateMachineError::AlreadyStarted;
        return false;
    }
    if (s.id.IsEmpty()) {
        last_error = StateMachineError::EmptyStateId;
        return false;
    }
    if (FindState(s.id)) {
        last_error = StateMachineError::DuplicateStateId;
        return false;
    }

    states.Add(MakeOne<State>(pick(s)));
    ClearError();
    return true;
}

//------------------------------------------------------------------------------
// Add a new transition definition
//------------------------------------------------------------------------------
bool StateMachine::AddTransition(Transition t) {
    if (started) {
        last_error = StateMachineError::AlreadyStarted;
        return false;
    }
    if (t.event.IsEmpty()) {
        last_error = StateMachineError::EmptyEvent;
        return false;
    }
    if (t.from.IsEmpty()) {
        last_error = StateMachineError::EmptyFromState;
        return false;
    }
    if (t.to.IsEmpty()) {
        last_error = StateMachineError::EmptyToState;
        return false;
    }

    if (!FindState(t.from)) {
        last_error = StateMachineError::MissingFromState;
        return false;
    }

    if (!FindState(t.to)) {
        last_error = StateMachineError::MissingToState;
        return false;
    }

    if (FindTransition(t.from, t.event)) {
        last_error = StateMachineError::DuplicateTransition;
        return false;
    }

    transitions.Add(MakeOne<Transition>(pick(t)));
    ClearError();
    return true;
}

//------------------------------------------------------------------------------
// Query helpers
//------------------------------------------------------------------------------
bool StateMachine::HasState(const String& id) const {
    return FindState(id) != nullptr;
}

bool StateMachine::HasTransition(const String& from, const String& event) const {
    return FindTransition(from, event) != nullptr;
}

int StateMachine::GetStateCount() const {
    return states.GetCount();
}

int StateMachine::GetTransitionCount() const {
    return transitions.GetCount();
}

//------------------------------------------------------------------------------
// Begin the state machine in its initial state
//------------------------------------------------------------------------------
bool StateMachine::Start() {
    if (initial.IsEmpty()) {
        last_error = StateMachineError::EmptyStateId;
        return false;
    }
    if (started) {
        last_error = StateMachineError::AlreadyStarted;
        return false;
    }
    if (transitioning) {
        last_error = StateMachineError::TransitionInProgress;
        return false;
    }

    const State* init = FindState(initial);
    if (!init) {
        last_error = StateMachineError::MissingState;
        return false;
    }

    const String start_initial = initial;
    auto start_finished = std::make_shared<bool>(false);
    started = true;
    transitioning = true;
    current = start_initial;
    ClearError();

    auto finish_start = [this, start_initial, start_finished](bool success) {
        if (*start_finished)
            return;
        *start_finished = true;

        if (success) {
            transitionHistory.Add(MakeOne<TransitionRecord>("", start_initial, "__start"));
            transitioning = false;
            ClearError();
            DrainQueuedEvents();
            return;
        }

        transitioning = false;
        started = false;
        current.Clear();
        transitionHistory.Clear();
        queued_events.Clear();
        last_error = StateMachineError::StartEnterFailed;
    };

    if (init->OnEnter)
        init->OnEnter(*this, [this, finish_start](bool success) {
            finish_start(success);
        });
    else
        finish_start(true);
    return true;
}

//------------------------------------------------------------------------------
// Trigger an event by name
//------------------------------------------------------------------------------
bool StateMachine::TriggerEvent(const String& e) {
    if (!started) {
        last_error = StateMachineError::NotStarted;
        return false;
    }
    if (transitioning) {
        switch (event_policy) {
        case EventPolicy::RejectWhileTransitioning:
            last_error = StateMachineError::EventRejectedWhileTransitioning;
            break;
        case EventPolicy::DropWhileTransitioning:
            last_error = StateMachineError::EventDroppedWhileTransitioning;
            break;
        case EventPolicy::QueueWhileTransitioning:
            return QueueEvent(e);
        }
        return false;
    }

    if (e.IsEmpty()) {
        last_error = StateMachineError::EmptyEvent;
        return false;
    }

    const Transition* t = FindTransition(current, e);
    if (!t) {
        last_error = StateMachineError::NoMatchingTransition;
        return false;
    }

    const State* from_state = FindState(t->from);
    const State* to_state = FindState(t->to);
    if (!from_state) {
        last_error = StateMachineError::MissingFromState;
        return false;
    }
    if (!to_state) {
        last_error = StateMachineError::MissingToState;
        return false;
    }

    TransitionContext ctx(*this, t->from, t->to, t->event);
    if (t->Guard && !t->Guard(ctx)) {
        last_error = StateMachineError::GuardRejected;
        return false;
    }

    if (!DoTransition(*t)) {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
// Attempt a transition by descriptor
//------------------------------------------------------------------------------
bool StateMachine::TryTransition(const Transition& t) {
    if (!started) {
        last_error = StateMachineError::NotStarted;
        return false;
    }
    if (transitioning) {
        last_error = StateMachineError::TransitionInProgress;
        return false;
    }

    if (t.event.IsEmpty()) {
        last_error = StateMachineError::EmptyEvent;
        return false;
    }

    if (t.from.IsEmpty()) {
        last_error = StateMachineError::EmptyFromState;
        return false;
    }

    if (t.to.IsEmpty()) {
        last_error = StateMachineError::EmptyToState;
        return false;
    }

    if (!FindState(t.from)) {
        last_error = StateMachineError::MissingFromState;
        return false;
    }

    if (!FindState(t.to)) {
        last_error = StateMachineError::MissingToState;
        return false;
    }

    if (!FindState(current)) {
        last_error = StateMachineError::MissingState;
        return false;
    }

    if (t.from != current) {
        last_error = StateMachineError::WrongSourceState;
        return false;
    }

    TransitionContext ctx(*this, t.from, t.to, t.event);
    if (t.Guard && !t.Guard(ctx)) {
        last_error = StateMachineError::GuardRejected;
        return false;
    }

    if (!DoTransition(t)) {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
// Revert to previous state if possible
//------------------------------------------------------------------------------
bool StateMachine::GoBack() {
    if (!started) {
        last_error = StateMachineError::NotStarted;
        return false;
    }
    if (IsTransitioning()) {
        last_error = StateMachineError::TransitionInProgress;
        return false;
    }
    if (!CanGoBack()) {
        last_error = StateMachineError::NoMatchingTransition;
        return false;
    }

    const One<TransitionRecord>& last_step = transitionHistory.Top();

    Transition back_transition;
    back_transition.from  = current;
    back_transition.to    = last_step->from;
    back_transition.event = "__back";

    bool began = DoTransition(back_transition, false, [this](bool success) {
        if (success) {
            transitionHistory.Pop();
            if (logging)
                DumpHistory();
            ClearError();
        }
        else {
            last_error = StateMachineError::BackTransitionFailed;
        }
    });
    if (!began)
        return false;
    return true;
}

//------------------------------------------------------------------------------
// Reset runtime state but keep configuration
//------------------------------------------------------------------------------
bool StateMachine::Reset() {
    if (IsTransitioning()) {
        last_error = StateMachineError::TransitionInProgress;
        return false;
    }

    current.Clear();
    started = false;
    transitioning = false;
    transitionHistory.Clear();
    queued_events.Clear();
    ClearError();
    return true;
}

//------------------------------------------------------------------------------
// Clear all runtime state and configuration
//------------------------------------------------------------------------------
bool StateMachine::Clear() {
    if (IsTransitioning()) {
        last_error = StateMachineError::TransitionInProgress;
        return false;
    }

    current.Clear();
    initial.Clear();
    started = false;
    transitioning = false;
    states.Clear();
    transitions.Clear();
    transitionHistory.Clear();
    queued_events.Clear();
    ClearError();
    return true;
}

void StateMachine::SetMaxQueuedEvents(int n) {
    if (n < 0)
        n = 0;
    max_queued_events = n;
    while (queued_events.GetCount() > max_queued_events)
        queued_events.Remove(queued_events.GetCount() - 1);
    ClearError();
}

//------------------------------------------------------------------------------
// Lookup helpers
//------------------------------------------------------------------------------
const State* StateMachine::FindState(const String& id) const {
    for (const auto& s : states)
        if (s->id == id)
            return s.Get();
    return nullptr;
}

const Transition* StateMachine::FindTransition(const String& from, const String& ev) const {
    for (const auto& t : transitions)
        if (t->from == from && t->event == ev)
            return t.Get();
    return nullptr;
}

bool StateMachine::DoTransition(const Transition& t,
                                bool record,
                                Function<void(bool)> on_done)
{
    if (logging)
        LOG(Format("DoTransition: %s -> %s, record=%d", t.from, t.to, int(record)));

    const State* fromState = FindState(t.from);
    const State* toState   = FindState(t.to);
    if (!fromState) {
        last_error = StateMachineError::MissingFromState;
        if (logging)
            LOG("Error: Transition specifies a missing from state.");
        if (on_done) on_done(false);
        return false;
    }
    if (!toState) {
        last_error = StateMachineError::MissingToState;
        if (logging)
            LOG("Error: Transition specifies a missing to state.");
        if (on_done) on_done(false);
        return false;
    }

    auto enter_finished = std::make_shared<bool>(false);
    auto exit_finished  = std::make_shared<bool>(false);
    auto enter_started  = std::make_shared<bool>(false);
    ClearError();
    transitioning = true;
    TransitionContext ctx(*this, t.from, t.to, t.event);

    // OnBefore callback
    if (WhenTransitionStarted)
        WhenTransitionStarted(ctx);
    if (t.OnBefore)
        t.OnBefore(ctx);

    // Chain exit → enter → finalize → after
    auto on_enter_done = [this, ctx, record, on_done, t, enter_finished, enter_started](bool success) {
        if (*enter_finished)
            return;
        *enter_finished = true;

        if (success) {
            Finalize(ctx, record);
            if (WhenTransitionFinished)
                WhenTransitionFinished(ctx);

            if (t.OnAfter)
                t.OnAfter(ctx);

            ClearError();
        }
        else {
            if (!record)
                last_error = StateMachineError::BackTransitionFailed;
            else if (*enter_started)
                last_error = StateMachineError::EnterFailed;
            else
                last_error = StateMachineError::ExitFailed;
        }
        transitioning = false;
        if (on_done) on_done(success);
        if (success)
            DrainQueuedEvents();
    };

    auto on_exit_done = [this, toState, ctx, on_enter_done, exit_finished, enter_started, enter_finished, record](bool success) {
        if (*exit_finished)
            return;
        *exit_finished = true;

        if (success) {
            if (toState && toState->OnEnter) {
                *enter_started = true;
                toState->OnEnter(*this, [this, ctx, on_enter_done, enter_finished](bool enter_success) {
                    if (*enter_finished)
                        return;
                    if (enter_success)
                        current = ctx.toState;
                    if (logging) {
                        LOG("Transition " + String(enter_success ? "succeeded" : "failed") +
                            ": now in state " + current);
                    }
                    on_enter_done(enter_success);
                });
            }
            else {
                *enter_started = true;
                current = ctx.toState;
                if (logging)
                    LOG("Transition succeeded: now in state " + current);
                on_enter_done(true);
            }
        }
        else {
            if (logging)
                LOG("Error: OnExit failed, transition aborted.");
            transitioning = false;
            last_error = record ? StateMachineError::ExitFailed : StateMachineError::BackTransitionFailed;
            on_enter_done(false);
        }
    };

    // Start exit phase
    if (fromState->OnExit)
        fromState->OnExit(*this, on_exit_done);
    else
        on_exit_done(true);

    return true;
}

bool StateMachine::QueueEvent(const String& e) {
    if (e.IsEmpty()) {
        last_error = StateMachineError::EmptyEvent;
        return false;
    }
    if (max_queued_events <= 0 || queued_events.GetCount() >= max_queued_events) {
        last_error = StateMachineError::EventQueueFull;
        return false;
    }
    queued_events.Add(e);
    ClearError();
    return true;
}

void StateMachine::DrainQueuedEvents() {
    if (processing_queue)
        return;

    processing_queue = true;
    const int drain_limit = max_queued_events > 0 ? max_queued_events : 0;
    int drain_steps = 0;
    while (!queued_events.IsEmpty() && started && !transitioning) {
        if (drain_steps >= drain_limit) {
            last_error = StateMachineError::EventQueueDrainLimitReached;
            break;
        }
        String event = queued_events[0];
        queued_events.Remove(0);
        ++drain_steps;
        if (!TriggerEvent(event))
            break;
        if (transitioning)
            break;
        if (last_error != StateMachineError::None)
            break;
    }
    processing_queue = false;
}

//------------------------------------------------------------------------------
// Core transition logic (handles OnExit→OnEnter→history→OnAfter)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Record history and dump if needed
//------------------------------------------------------------------------------
void StateMachine::Finalize(const TransitionContext& ctx, bool record) {
    if (logging)
        LOG(Format("Finalize: %s -> %s, record=%d", ctx.fromState, ctx.toState, int(record)));

    if (record) {
        // prune any divergent history
        while (!transitionHistory.IsEmpty() &&
               transitionHistory.Top()->to != ctx.fromState)
        {
            transitionHistory.Pop();
        }
        transitionHistory.Add(
            MakeOne<TransitionRecord>(ctx.fromState, ctx.toState, ctx.event));
        if (logging)
            DumpHistory();
    }
}

} // namespace Upp
