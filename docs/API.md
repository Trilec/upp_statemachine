# API

## Public files

- `statemachine/statemachine.h`
- `statemachine/statemachine.cpp`

## Types

### `TransitionContext`

Carries the live transition data into callbacks.

- `StateMachine& machine`
- `String fromState`
- `String toState`
- `String event`

### `State`

Defines one state in the machine.

- `String id`
- `Function<void(StateMachine&, Function<void(bool)> done)> OnEnter`
- `Function<void(StateMachine&, Function<void(bool)> done)> OnExit`

### `Transition`

Defines an event-driven path between two states.

- `String event`
- `String from`
- `String to`
- `Function<bool(const TransitionContext&)> Guard`
- `Function<void(const TransitionContext&)> OnBefore`
- `Function<void(const TransitionContext&)> OnAfter`

### `TransitionRecord`

Stores a completed transition for history and `GoBack()`.

- `String from`
- `String to`
- `String event`

### `EventPolicy`

Controls how events are treated while a transition is already in progress.

- `RejectWhileTransitioning`
- `DropWhileTransitioning`
- `QueueWhileTransitioning`

## `StateMachine`

### Configuration

- `SetInitial(const String& id) -> bool`
- `GetInitial() const`
- `HasInitial() const`
- `AddState(State s) -> bool`
- `AddTransition(Transition t) -> bool`
- `SetEventPolicy(EventPolicy policy)`
- `GetEventPolicy() const`
- `SetMaxQueuedEvents(int n)`
- `GetMaxQueuedEvents() const`
- `GetQueuedEventCount() const`
- `HasQueuedEvents() const`
- `ClearQueuedEvents()`

### Execution

- `Start() -> bool`
- `TriggerEvent(const String& e) -> bool`
- `TryTransition(const Transition& t) -> bool`
- `GoBack() -> bool`
- `Reset() -> bool`
- `Clear() -> bool`

### Errors

- `StateMachineError GetLastError() const`
- `String GetLastErrorText() const`
- `void ClearError()`

Relevant transition-time event errors:

- `EventRejectedWhileTransitioning`
- `EventDroppedWhileTransitioning`
- `EventQueueFull`

### Logging

- `EnableLogging(bool b = true)`
- `IsLoggingEnabled() const`

### Queries

- `GetCurrent() const`
- `IsStarted() const`
- `IsTransitioning() const`
- `CanGoBack() const`
- `HasState(const String& id) const`
- `HasTransition(const String& from, const String& event) const`
- `GetStateCount() const`
- `GetTransitionCount() const`
- `GetHistoryCount() const`
- `GetHistoryFrom(int i) const`
- `GetHistoryTo(int i) const`
- `GetHistoryEvent(int i) const`

### Hooks

- `WhenTransitionStarted`
- `WhenTransitionFinished`

## TriggerEvent(event)

`TriggerEvent()` returns `true` when a transition begins and `false` when it
is ignored or blocked.

1. If the machine is already transitioning, the stored `EventPolicy` is applied.
2. If the machine has not been started, the event is ignored.
3. The machine searches for a transition where:
   - `transition.from == current state`
   - `transition.event == event`
4. If the source state or target state does not exist, nothing happens.
5. If a guard exists and returns false, nothing happens.
6. `TriggerEvent()` only returns `true` after the source and target have been
   validated and the guard has allowed the transition.
7. `WhenTransitionStarted` is called.
8. The transition `OnBefore` callback is called.
9. The current state's `OnExit` callback is called.
10. The target state's `OnEnter` callback is called.
11. If `OnEnter` succeeds, the current state is updated.
12. The transition is recorded in history.
13. `WhenTransitionFinished` is called.
14. The transition `OnAfter` callback is called.
15. The transitioning flag is cleared.
16. If the transition succeeded, queued event names are drained in FIFO order.

If `TriggerEvent()` is called while a transition is in progress, the stored
policy determines the error:

- `RejectWhileTransitioning` -> `EventRejectedWhileTransitioning`
- `DropWhileTransitioning` -> `EventDroppedWhileTransitioning`
- `QueueWhileTransitioning` -> queue the event name and return `true`
- `QueueWhileTransitioning` when full -> `EventQueueFull`

When `TriggerEvent()` fails, `GetLastError()` and `GetLastErrorText()` report
the reason.

`QueueWhileTransitioning` is intentionally lightweight: queued `TriggerEvent()`
names only, bounded FIFO order, no queued `TryTransition()`, no queued
`GoBack()`, no cancellation, and no hierarchy.

## Callback order

For a successful normal transition, the current tested order is:

1. `WhenTransitionStarted`
2. `OnBefore`
3. `OnExit`
4. `OnEnter`
5. `WhenTransitionFinished`
6. `OnAfter`

At `WhenTransitionFinished` and `OnAfter`, `GetCurrent()` already returns the
target state, the new history entry is present, and `IsTransitioning()` is
still `true` until the transition callback chain unwinds.

Observed state by callback phase for a successful normal transition:

- `Guard`: `GetCurrent() == source`, `IsStarted() == true`, `IsTransitioning() == false`
- `WhenTransitionStarted`: `GetCurrent() == source`, `IsStarted() == true`, `IsTransitioning() == true`
- `OnBefore`: `GetCurrent() == source`, `IsStarted() == true`, `IsTransitioning() == true`
- `OnExit`: `GetCurrent() == source`, `IsStarted() == true`, `IsTransitioning() == true`
- `OnEnter`: `GetCurrent() == source`, `IsStarted() == true`, `IsTransitioning() == true`
- If `OnEnter` later calls `done(false)`, `GetCurrent()` rolls back to the
  source state, no history entry is committed, `IsTransitioning()` becomes
  `false`, `GetLastError() == EnterFailed`, and `WhenTransitionFinished` /
  `OnAfter` do not run.
- `WhenTransitionFinished`: `GetCurrent() == target`, `IsStarted() == true`, `IsTransitioning() == true`
- `OnAfter`: `GetCurrent() == target`, `IsStarted() == true`, `IsTransitioning() == true`
- After completion: `GetCurrent() == target`, `IsStarted() == true`, `IsTransitioning() == false`

## TryTransition(t)

`TryTransition()` returns `true` when a transition begins and `false` when it
is rejected.

It returns `false` if:

- the machine has not been started
- the machine is already transitioning
- `t.from != current`
- `t.to` does not exist
- the guard blocks the transition

If `TryTransition()` succeeds, `OnAfter` is called from the exact transition
object passed into `DoTransition()`.

When `TryTransition()` fails, `GetLastError()` and `GetLastErrorText()`
report the reason.

`true` means the operation was accepted or began. It does not mean async work
has completed.

## Start()

`Start()` returns `true` when startup begins successfully and `false` when:

- the initial state is empty
- the initial state does not exist
- the machine has already been started
- the machine is currently transitioning

On success:

- `transitioning` is set to `true` before initial `OnEnter` runs
- `started` becomes `true`
- `current` is set to the initial state
- a `__start` history record is added

`Start()` returning `true` means startup has been accepted.
`IsStarted()` means `Start()` has been accepted and the machine owns a current
initial state.
`IsTransitioning()` means startup or a transition is currently in progress.

If the initial `OnEnter` is asynchronous, startup may still be in progress
after `Start()` returns `true`. During that time `IsStarted()` is `true`,
`IsTransitioning()` is `true`, and `GetCurrent() == initial`.

If the initial `OnEnter` later fails, startup rolls back and `IsStarted()`
becomes `false`.

Machine-owned completion callbacks are single-shot: the first `done(true/false)`
wins and duplicate completions are ignored.

`true` means startup was accepted. It does not mean the async work has
completed.

The current implementation assumes same-thread, same-callback-chain use unless
the API explicitly says otherwise. Pending async callbacks hold a live
reference to the machine, so the `StateMachine` object must outlive them.

When startup fails, `GetLastError()` and `GetLastErrorText()` report the
reason.

If the initial state has no `OnEnter`, startup completes immediately with
`started == true`, `current == initial`, and `transitioning == false`.

## AddState(state)

`AddState()` returns `true` when the state is accepted and `false` when:

- `state.id` is empty
- `state.id` already exists
- the machine has already been started

When `AddState()` fails, `GetLastError()` and `GetLastErrorText()` report the
reason.

Example:

```cpp
StateMachine sm;
if(!sm.AddState({"Idle", {}, {}}))
    return;
if(!sm.SetInitial("Idle"))
    return;
if(!sm.Start())
    LOG(sm.GetLastErrorText());
```

## History accessors

The history accessors are read-only helpers for tests and diagnostics.

- `GetHistoryCount()` returns the number of recorded history entries.
- `GetHistoryFrom(i)` returns the `from` field for entry `i`, or an empty
  `String` if `i` is invalid.
- `GetHistoryTo(i)` returns the `to` field for entry `i`, or an empty `String`
  if `i` is invalid.
- `GetHistoryEvent(i)` returns the `event` field for entry `i`, or an empty
  `String` if `i` is invalid.

## Query helpers

These are read-only helpers for tests, diagnostics, and later tooling.

- `HasState(id)` returns whether a state id exists.
- `HasTransition(from, event)` returns whether that transition key exists.
- `GetStateCount()` returns the number of configured states.
- `GetTransitionCount()` returns the number of configured transitions.

## Logging

Logging is disabled by default.

- `EnableLogging(true)` turns on internal transition logging.
- `EnableLogging(false)` turns it off again.
- Normal transitions and transition errors stay quiet unless logging is enabled.
- `DumpHistory()` still logs explicitly when called.

## AddTransition(transition)

`AddTransition()` returns `true` when the transition is accepted and `false`
when:

- `transition.event` is empty
- `transition.from` is empty
- `transition.to` is empty
- `transition.from` does not exist
- `transition.to` does not exist
- a transition with the same `from` and `event` already exists
- the machine has already been started

When `AddTransition()` fails, `GetLastError()` and `GetLastErrorText()`
report the reason.

Example:

```cpp
StateMachine sm;
if(!sm.AddState({"Idle", {}, {}}))
    return;
if(!sm.AddState({"Working", {}, {}}))
    return;
if(!sm.AddTransition({"start", "Idle", "Working"}))
    return;
```

## GoBack()

`GoBack()` returns `true` when a back transition begins and `false` when it is
rejected.

It returns `false` if:

- the machine has not been started
- the machine is already transitioning
- there is no prior history entry to return to

When `GoBack()` fails, `GetLastError()` and `GetLastErrorText()` report the
reason.

## Reset()

`Reset()` returns `true` when runtime state is cleared and configuration is
kept, and `false` when the machine is transitioning.

On success, `Reset()` clears:

- `current`
- `started`
- `transitioning`
- history
- last error

It does not clear:

- `initial`
- states
- transitions

## Clear()

`Clear()` returns `true` when the machine is fully cleared and `false` when it
is transitioning.

On success, `Clear()` clears:

- `current`
- `initial`
- `started`
- `transitioning`
- states
- transitions
- history
- last error

## Boolean contract

For this API, `true` generally means the operation was accepted or began
successfully. It does not necessarily mean any asynchronous work has already
finished.

## Current limitations

- Events fired during a transition are ignored, not queued.
- `Start()` uses a synthetic `__start` history record instead of a normal transition event.
- Async callbacks capture the `StateMachine` object. The caller must ensure the `StateMachine` outlives pending callbacks.
- `GoBack()` uses a synthetic direct transition named `"__back"`.
- `GoBack()` does not require a registered reverse transition.
- Event queueing is not implemented.
- Transition cancellation is not implemented.
