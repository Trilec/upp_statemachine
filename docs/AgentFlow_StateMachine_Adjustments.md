# AgentFlow State Machine Adjustments

## Implementation brief

**Task ID:** `SM-002`

## Repository

```text
Trilec/upp_statemachine
```

## Required starting baseline

```text
Branch: main
Commit: 93e20f109181e16667176214484cb4f585a3d2b8
Version baseline: v1.0.1
Authoritative core test baseline: 190/190
```

Before changing code, confirm the branch, commit, working tree and complete existing test/build status. If the starting commit differs, stop and report the exact difference before implementing.

Recommended working branch:

```text
feature/async-lifecycle-hardening
```

Create a local commit only. Do not push.

---

## Objective

Strengthen the existing `StateMachine` so it can safely control long-lived asynchronous orchestration without changing its identity as a compact, Core-only, single-current-state FSM.

The task must add:

- explicit cancellation of an active asynchronous transition;
- stale and late completion protection;
- safe callback behavior after machine destruction;
- structured terminal transition outcomes;
- stable transition sequence identity;
- exact-once settlement for startup, normal transitions and `GoBack()`.

This is an additive lifecycle hardening task for a future `v1.1.0`. It is not a graph scheduler, agent framework or state-machine redesign.

---

## Architectural constraints

Preserve all of the following:

- `statemachine` depends only on `Core`;
- same-thread / same-callback-chain contract;
- no internal worker thread;
- no mutex-based attempt to disguise cross-thread use;
- flat states;
- explicit event transitions;
- current guards and hooks;
- existing `done(bool)` handler compatibility;
- current return convention where `true` means accepted/begun, not necessarily completed;
- current successful transition observability;
- current queue policies;
- current rollback-before-commit semantics;
- current history behavior;
- quiet logging unless enabled.

Do not add:

- graph nodes or ports;
- parallel state regions;
- workflow scheduling;
- event payload queues;
- agent concepts;
- GUI dependencies;
- timer ownership;
- model-provider code;
- tool execution;
- hidden global lifetime registries.

Timeout policy belongs to the caller. A caller may cancel a transition because its own timeout expired, but the state machine must not introduce an internal timer framework in this task.

---

## Required inspection before implementation

Inspect the complete current code and documentation, including:

```text
statemachine/statemachine.h
statemachine/statemachine.cpp
tests/StateMachineCoreTest/main.cpp
examples/StateMachineGuiTest/
examples/StateMachineVisualizer/
docs/API.md
docs/DESIGN.md
README.md
CHANGELOG.md
```

Do not work only from this task description or only from the previous commit.

Document the existing callback and hook ordering before changing it.

---

## Core design requirement

The current implementation protects a completion callback from being invoked twice during one transition, but local lambdas still capture the machine and an old callback can outlive or outdate the active operation.

Replace this with an explicit active-operation lifecycle.

A sound internal design will normally include:

- one monotonically increasing transition sequence;
- one active-operation record;
- one exact-once terminal settlement gate;
- a machine lifetime token held weakly by completion callbacks;
- active sequence validation before any callback changes machine state;
- an explicit operation kind:
  - startup;
  - normal transition;
  - back transition.

Do not merely add another shared Boolean around the existing lambdas.

The final implementation may choose exact internal names, but it must make cancellation, stale completion and destruction behavior mechanically enforceable.

---

## Public API direction

The implementation should provide an API equivalent in capability to the following. Exact naming may be refined to fit the existing API vocabulary, but explain any deviation.

```cpp
enum class TransitionOutcome {
    None,
    Succeeded,
    FailedExit,
    FailedEnter,
    FailedStart,
    FailedBack,
    Cancelled
};

enum class TransitionOperationKind {
    None,
    Start,
    Transition,
    Back
};

struct TransitionResult {
    uint64 sequence = 0;
    TransitionOperationKind operation = TransitionOperationKind::None;
    TransitionOutcome outcome = TransitionOutcome::None;
    String from;
    String to;
    String event;
};

bool CancelActiveTransition();
bool HasActiveTransition() const;
uint64 GetActiveTransitionSequence() const;
uint64 GetLastSettledTransitionSequence() const;
TransitionOutcome GetLastTransitionOutcome() const;
const TransitionResult& GetLastTransitionResult() const;

Function<void(const TransitionResult&)> WhenTransitionSettled;
```

Required compatibility behavior:

- existing state `OnEnter` and `OnExit` callbacks remain `done(bool)`;
- existing `WhenTransitionStarted` remains;
- existing `WhenTransitionFinished` remains success-only;
- existing transition-specific `OnAfter` remains success-only;
- existing public methods retain their current broad meaning.

Add a stable error for attempting cancellation when no transition is active, for example:

```cpp
StateMachineError::NoActiveTransition
```

Do not report a successfully accepted cancellation as a public-call failure. Cancellation is represented by `TransitionOutcome::Cancelled`, not by leaving a misleading failure error after `CancelActiveTransition()` returns `true`.

---

## Required settlement ordering

### Successful normal transition

Preserve the existing successful contract:

1. guard;
2. transition becomes active;
3. `WhenTransitionStarted`;
4. transition `OnBefore`;
5. source `OnExit`;
6. target `OnEnter`;
7. target becomes current;
8. history commits;
9. `WhenTransitionFinished`;
10. transition `OnAfter`;
11. transition becomes inactive;
12. `WhenTransitionSettled(Succeeded)`;
13. queued events may drain.

Existing successful hooks must continue to observe the target current state and committed history while the transition is still active, unless the current documented contract is intentionally changed and fully justified. The preferred result is to preserve it.

### Failed transition

- current state remains the source;
- history does not commit;
- success-only hooks do not fire;
- transition becomes inactive;
- exactly one structured failed settlement fires;
- existing failure error remains authoritative;
- queued events do not drain.

### Cancelled normal transition

- current state remains the logical source;
- history does not commit;
- success-only hooks do not fire;
- transition becomes inactive;
- exactly one cancelled settlement fires;
- queued events remain intact;
- queued events do not auto-drain;
- any later exit/enter completion for the cancelled sequence is ignored.

The state machine cannot automatically reverse external side effects already performed by a user callback. Document this explicitly.

### Cancelled startup

- `IsStarted()` becomes false;
- current state clears;
- startup history clears;
- queued startup events clear, matching failed-start rollback;
- exactly one cancelled settlement fires;
- a later initial `OnEnter` completion is ignored.

### Cancelled `GoBack()`

- current state remains unchanged;
- history remains unchanged;
- no `BackTransitionFailed` is produced merely because the operation was cancelled;
- exactly one cancelled settlement fires;
- a later completion is ignored.

---

## Lifetime safety

A completion callback retained by client code must be safe to invoke after the `StateMachine` has been destroyed.

Required result:

- no dereference of a destroyed machine;
- no crash;
- no callback into dead user hooks;
- no global object required;
- no memory leak caused by a callback retaining the entire machine indefinitely.

A suitable design is a machine-owned shared lifetime record containing an owner pointer or generation, while callbacks hold only a weak reference. The exact implementation may differ, but raw `this` must not be the only lifetime defence.

Review copy and move behavior. Do not accidentally make a machine movable while pending callbacks still point to its old address. Preserve or explicitly constrain the existing practical behavior and document it.

---

## Stale completion rules

A completion is stale when its sequence is no longer the active sequence because the transition was:

- cancelled;
- superseded by reset/clear after cancellation;
- followed by a newer transition;
- already settled;
- associated with a destroyed machine.

A stale completion must not:

- change `current`;
- change `started`;
- change `transitioning`;
- change history;
- alter queued events;
- overwrite the last error;
- overwrite the last settled result;
- fire success hooks;
- fire settlement again;
- start queue draining.

Optional diagnostic logging is allowed only when normal logging is enabled.

---

## Reset and clear behavior

Continue rejecting `Reset()` and `Clear()` while a transition is active.

After a transition has been cancelled and settled:

- `Reset()` may succeed;
- `Clear()` may succeed;
- an old completion invoked afterward must remain harmless.

Do not make `Reset()` or `Clear()` implicitly cancel an active transition in this task.

---

## Queue behavior

Preserve all current queue tests and semantics.

Add explicit tests showing:

- queued events survive cancellation of a normal transition;
- cancellation does not drain them;
- failed startup cancellation clears queued startup events;
- a later successful transition can still cause retained queued events to drain according to the current policy;
- stale completion cannot drain the queue;
- queue capacity and drain-cycle protections remain unchanged.

If an existing queue invariant conflicts with this contract, report it before changing behavior.

---

## Exact-once requirements

For every operation:

- terminal settlement occurs at most once;
- completion callback may be invoked zero, one or many times by client code without corrupting state;
- cancellation may race logically with a stored completion callback on the same thread;
- the first accepted terminal action wins;
- all later terminal attempts are ignored;
- `WhenTransitionSettled` fires exactly once;
- success-only hooks remain exactly once on success.

---

## Required tests

Add focused sections to the authoritative core suite.

At minimum cover:

### Startup

- synchronous startup succeeds;
- asynchronous startup succeeds;
- asynchronous startup fails;
- cancel during startup;
- repeated cancel during/after startup;
- late success after cancelled startup;
- late failure after cancelled startup;
- callback invoked after machine destruction.

### Normal transition exit phase

- cancel before exit completion;
- source remains current;
- history unchanged;
- settlement exactly once;
- late exit success ignored;
- late exit failure ignored;
- double exit completion ignored.

### Normal transition enter phase

- exit succeeds and enter remains pending;
- cancel during enter;
- source remains current;
- history unchanged;
- late enter success ignored;
- late enter failure ignored;
- double enter completion ignored.

### New transition after cancellation

- cancel transition A;
- begin and complete transition B;
- invoke A’s stored completion;
- B’s state, history, outcome and error remain unchanged.

### GoBack

- cancel pending back transition;
- state/history unchanged;
- no false back-failure result;
- late back completion ignored.

### Queueing

- retained normal-transition queue;
- no automatic drain on cancellation;
- startup queue clearing;
- no stale-callback drain;
- existing queue matrix still passes.

### Hooks and results

- `WhenTransitionStarted` once;
- `WhenTransitionFinished` success-only;
- `OnAfter` success-only;
- `WhenTransitionSettled` exactly once for success, failure and cancellation;
- sequence increases monotonically;
- last settled result is stable against stale callbacks;
- `CancelActiveTransition()` with no active transition returns false and reports the new stable error.

### Lifecycle

- pending callback after destructor is harmless;
- pending callback after cancel then reset is harmless;
- pending callback after cancel then clear is harmless.

Use deterministic tests. Do not add sleeps, threads or timing-dependent tests.

---

## Documentation

Update:

```text
README.md
docs/API.md
docs/DESIGN.md
CHANGELOG.md
```

Document:

- why this is an additive v1.1 hardening;
- cancellation semantics;
- structured outcome semantics;
- hook ordering;
- late callback behavior;
- same-thread requirement;
- external side effects are not automatically reversed;
- timeout remains caller-owned;
- object lifetime behavior;
- queue behavior after cancellation.

Do not claim thread safety.

---

## Build and validation

Run, sequentially:

```text
StateMachineCoreTest
StateMachineGuiTest build
StateMachineVisualizer build
```

Launch the visualizer as a smoke check if the local environment supports it.

The existing baseline of `190/190` must remain green, with new checks added on top.

Inspect the final complete diff and all affected code, not only the new tests.

---

## Non-goals

Do not implement:

- hierarchical states;
- parallel states;
- graph execution;
- agent orchestration;
- transition payloads;
- internal timers;
- background workers;
- mutexes;
- cancellation tokens for arbitrary external systems;
- GUI controls;
- compatibility aliases with no proven need.

---

## Completion report

Report:

- starting branch and commit;
- branch used;
- architectural summary;
- public API changes;
- exact cancellation and settlement ordering;
- lifetime design;
- changed files;
- test counts;
- build commands and outcomes;
- compatibility findings;
- known limitations;
- local commit SHA.

Use a commit title no longer than 90 characters and a concise 2–7 line body.

Suggested title:

```text
StateMachine hardens async cancellation and stale completion
```
