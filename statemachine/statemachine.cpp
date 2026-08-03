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
    case StateMachineError::NoActiveTransition: return "No active transition";
    }
    return "Unknown error";
}

String StateMachine::GetLastErrorText() const {
    return GetStateMachineErrorText(last_error);
}

struct StateMachine::Lifetime : Pte<Lifetime> {
    StateMachine* owner = nullptr;
};

struct StateMachine::Operation : Pte<Operation> {
    enum class Phase { Active, Claiming, Settled };
    TransitionResult result;
    Phase phase = Phase::Active;
    bool record = true;
    CallbackPhase awaiting = CallbackPhase::Start;
    StateMachine* owner = nullptr;
    Function<void(const TransitionContext&)> on_after;
};

struct StateMachine::Completion {
    Ptr<Operation> operation;
    CallbackPhase phase;

    Completion(Operation* operation, CallbackPhase phase)
        : operation(operation), phase(phase) {}

    void operator()(bool success) const {
        if (operation && operation->owner)
            operation->owner->CompleteOperation(operation, phase, success);
    }
};

StateMachine::StateMachine() : lifetime(MakeOne<Lifetime>()) {
    lifetime->owner = this;
}

StateMachine::~StateMachine() {
    if (lifetime)
        lifetime->owner = nullptr;
    active_operation.Clear();
    lifetime.Clear();
}

uint64 StateMachine::GetActiveTransitionSequence() const {
    return active_operation ? active_operation->result.sequence : 0;
}

bool StateMachine::IsCurrent(const Operation* op) const {
    return op && active_operation.Get() == op && op->phase == Operation::Phase::Active;
}

bool StateMachine::IsClaimed(const Operation* op) const {
    return op && active_operation.Get() == op && op->phase == Operation::Phase::Claiming;
}

bool StateMachine::Claim(Operation* op, TransitionOutcome outcome) {
    if (!IsCurrent(op))
        return false;
    op->phase = Operation::Phase::Claiming;
    op->result.outcome = outcome;
    return true;
}

void StateMachine::SettleClaimed(Operation* op, bool drain_queue) {
    if (!op || op->phase != Operation::Phase::Claiming || active_operation.Get() != op)
        return;
    op->phase = Operation::Phase::Settled;
    last_result = op->result;
    transitioning = false;
    const TransitionResult stable_result = op->result;
    Ptr<Lifetime> self(lifetime.Get());
    active_operation.Clear();
    auto settled_callback = WhenTransitionSettled;
    if (settled_callback)
        settled_callback(stable_result);
    StateMachine* owner = self ? self->owner : nullptr;
    if (owner && drain_queue)
        owner->DrainQueuedEvents();
}

bool StateMachine::CancelActiveTransition() {
    if (!active_operation) {
        last_error = StateMachineError::NoActiveTransition;
        return false;
    }
    Operation* op = active_operation.Get();
    if (!Claim(op, TransitionOutcome::Cancelled)) {
        return false;
    }
    if (op->result.operation == TransitionOperationKind::Start) {
        started = false;
        current.Clear();
        transitionHistory.Clear();
        queued_events.Clear();
    }
    ClearError();
    SettleClaimed(op, false);
    return true;
}

void StateMachine::CompleteOperation(Operation* op, CallbackPhase phase, bool success) {
    if (!IsCurrent(op) || op->awaiting != phase)
        return;

    if (phase == CallbackPhase::Start) {
        if (success) {
            if (!Claim(op, TransitionOutcome::Succeeded))
                return;
            transitionHistory.Add(MakeOne<TransitionRecord>("", op->result.to, "__start"));
            ClearError();
            SettleClaimed(op, true);
        }
        else {
            if (!Claim(op, TransitionOutcome::FailedStart))
                return;
            started = false;
            current.Clear();
            transitionHistory.Clear();
            queued_events.Clear();
            last_error = StateMachineError::StartEnterFailed;
            SettleClaimed(op, false);
        }
        return;
    }

    if (phase == CallbackPhase::Exit) {
        if (!success) {
            if (!Claim(op, op->result.operation == TransitionOperationKind::Back ? TransitionOutcome::FailedBack : TransitionOutcome::FailedExit))
                return;
            last_error = op->result.operation == TransitionOperationKind::Back ? StateMachineError::BackTransitionFailed : StateMachineError::ExitFailed;
            SettleClaimed(op, false);
            return;
        }

        const State* target = FindState(op->result.to);
        if (!target) {
            if (Claim(op, op->result.operation == TransitionOperationKind::Back ? TransitionOutcome::FailedBack : TransitionOutcome::FailedEnter)) {
                last_error = StateMachineError::MissingToState;
                SettleClaimed(op, false);
            }
            return;
        }
        op->awaiting = CallbackPhase::Enter;
        auto enter_callback = target->OnEnter;
        if (enter_callback)
            enter_callback(*this, Function<void(bool)>(Completion(op, CallbackPhase::Enter)));
        else
            CompleteOperation(op, CallbackPhase::Enter, true);
        return;
    }

    if (!success) {
        if (!Claim(op, op->result.operation == TransitionOperationKind::Back ? TransitionOutcome::FailedBack : TransitionOutcome::FailedEnter))
            return;
        last_error = op->result.operation == TransitionOperationKind::Back ? StateMachineError::BackTransitionFailed : StateMachineError::EnterFailed;
        SettleClaimed(op, false);
        return;
    }

    if (!Claim(op, TransitionOutcome::Succeeded))
        return;
    TransitionContext ctx(*this, op->result.from, op->result.to, op->result.event);
    current = ctx.toState;
    Finalize(ctx, op->record);
    if (op->result.operation == TransitionOperationKind::Back && !transitionHistory.IsEmpty())
        transitionHistory.Pop();
    ClearError();

    Ptr<Lifetime> finished_lifetime(lifetime.Get());
    auto finished_callback = WhenTransitionFinished;
    if (finished_callback)
        finished_callback(ctx);
    StateMachine* owner = finished_lifetime ? finished_lifetime->owner : nullptr;
    if (!owner || !owner->IsClaimed(op))
        return;
    Ptr<Lifetime> after_lifetime(owner->lifetime.Get());
    auto after_callback = op->on_after;
    if (after_callback)
        after_callback(ctx);
    owner = after_lifetime ? after_lifetime->owner : nullptr;
    if (!owner || !owner->IsClaimed(op))
        return;
    owner->ClearError();
    owner->SettleClaimed(op, true);
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
    ++runtime_generation;
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
    ++runtime_generation;
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
    auto op = MakeOne<Operation>();
    op->result.sequence = ++next_sequence;
    op->result.operation = TransitionOperationKind::Start;
    op->result.to = start_initial;
    op->result.event = "__start";
    op->owner = this;
    active_operation = pick(op);
    Operation* active = active_operation.Get();
    started = true;
    transitioning = true;
    current = start_initial;
    ++runtime_generation;
    ClearError();

    auto initial_enter = init->OnEnter;
    Function<void(bool)> completion(Completion(active, CallbackPhase::Start));
    if (initial_enter)
        initial_enter(*this, completion);
    else
        CompleteOperation(active, CallbackPhase::Start, true);
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

    const Transition* found = FindTransition(current, e);
    if (!found) {
        last_error = StateMachineError::NoMatchingTransition;
        return false;
    }

    const Transition t = *found;
    const State* from_state = FindState(t.from);
    const State* to_state = FindState(t.to);
    if (!from_state) {
        last_error = StateMachineError::MissingFromState;
        return false;
    }
    if (!to_state) {
        last_error = StateMachineError::MissingToState;
        return false;
    }

    const String source_before_guard = current;
    const uint64 guard_ticket = ++guard_generation;
    const uint64 runtime_ticket = runtime_generation;
    Ptr<Lifetime> guard_lifetime(lifetime.Get());
    auto guard = t.Guard;
    TransitionContext ctx(*this, t.from, t.to, t.event);
    const bool allowed = !guard || guard(ctx);
    if (!guard_lifetime || guard_lifetime->owner != this || guard_generation != guard_ticket ||
        runtime_generation != runtime_ticket ||
        !started || transitioning || current != source_before_guard || current != t.from)
        return false;

    if (!allowed) {
        last_error = StateMachineError::GuardRejected;
        return false;
    }

    if (!DoTransition(t)) {
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

    const Transition transition = t;
    if (transition.event.IsEmpty()) {
        last_error = StateMachineError::EmptyEvent;
        return false;
    }

    if (transition.from.IsEmpty()) {
        last_error = StateMachineError::EmptyFromState;
        return false;
    }

    if (transition.to.IsEmpty()) {
        last_error = StateMachineError::EmptyToState;
        return false;
    }

    if (!FindState(transition.from)) {
        last_error = StateMachineError::MissingFromState;
        return false;
    }

    if (!FindState(transition.to)) {
        last_error = StateMachineError::MissingToState;
        return false;
    }

    if (!FindState(current)) {
        last_error = StateMachineError::MissingState;
        return false;
    }

    if (transition.from != current) {
        last_error = StateMachineError::WrongSourceState;
        return false;
    }

    const String source_before_guard = current;
    const uint64 guard_ticket = ++guard_generation;
    const uint64 runtime_ticket = runtime_generation;
    Ptr<Lifetime> guard_lifetime(lifetime.Get());
    auto guard = transition.Guard;
    TransitionContext ctx(*this, transition.from, transition.to, transition.event);
    const bool allowed = !guard || guard(ctx);
    if (!guard_lifetime || guard_lifetime->owner != this || guard_generation != guard_ticket ||
        runtime_generation != runtime_ticket ||
        !started || transitioning || current != source_before_guard || current != transition.from)
        return false;

    if (!allowed) {
        last_error = StateMachineError::GuardRejected;
        return false;
    }

    if (!DoTransition(transition)) {
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

    bool began = DoTransition(back_transition, false, TransitionOperationKind::Back);
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
    ++runtime_generation;
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
    ++runtime_generation;
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

bool StateMachine::DoTransition(const Transition& t, bool record, TransitionOperationKind kind)
{
    if (logging)
        LOG(Format("DoTransition: %s -> %s, record=%d", t.from, t.to, int(record)));

    const Transition transition = t;
    const State* fromState = FindState(transition.from);
    const State* toState   = FindState(transition.to);
    if (!fromState) {
        last_error = StateMachineError::MissingFromState;
        if (logging)
            LOG("Error: Transition specifies a missing from state.");
        return false;
    }
    if (!toState) {
        last_error = StateMachineError::MissingToState;
        if (logging)
            LOG("Error: Transition specifies a missing to state.");
        return false;
    }

    auto op = MakeOne<Operation>();
    op->result.sequence = ++next_sequence;
    op->result.operation = kind;
    op->result.from = transition.from;
    op->result.to = transition.to;
    op->result.event = transition.event;
    op->record = record;
    op->awaiting = CallbackPhase::Exit;
    op->owner = this;
    op->on_after = transition.OnAfter;
    active_operation = pick(op);
    Operation* active = active_operation.Get();
    ClearError();
    transitioning = true;
    ++runtime_generation;
    TransitionContext ctx(*this, transition.from, transition.to, transition.event);

    Ptr<Lifetime> callback_lifetime(lifetime.Get());
    auto started_callback = WhenTransitionStarted;
    if (started_callback)
        started_callback(ctx);
    if (!callback_lifetime || callback_lifetime->owner != this || !IsCurrent(active))
        return true;
    auto before_callback = transition.OnBefore;
    if (before_callback)
        before_callback(ctx);
    if (!callback_lifetime || callback_lifetime->owner != this || !IsCurrent(active))
        return true;

    // Chain exit → enter → finalize → after
    auto exit_callback = fromState->OnExit;
    if (exit_callback)
        exit_callback(*this, Function<void(bool)>(Completion(active, CallbackPhase::Exit)));
    else
        CompleteOperation(active, CallbackPhase::Exit, true);

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
    Ptr<Lifetime> drain_lifetime(lifetime.Get());
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
        const uint64 sequence_before = next_sequence;
        if (!TriggerEvent(event)) {
            if (!drain_lifetime || drain_lifetime->owner != this)
                return;
            break;
        }
        if (!drain_lifetime || drain_lifetime->owner != this)
            return;
        if (transitioning)
            break;
        if (last_result.sequence > sequence_before && last_result.outcome == TransitionOutcome::Cancelled)
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
