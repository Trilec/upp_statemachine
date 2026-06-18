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

## `StateMachine`

### Configuration

- `SetInitial(const String& id)`
- `AddState(State s)`
- `AddTransition(Transition t)`

### Execution

- `Start() -> bool`
- `TriggerEvent(const String& e) -> bool`
- `TryTransition(const Transition& t) -> bool`
- `GoBack()`

### Queries

- `GetCurrent() const`
- `IsStarted() const`
- `IsTransitioning() const`
- `CanGoBack() const`

### Hooks

- `WhenTransitionStarted`
- `WhenTransitionFinished`

## TriggerEvent(event)

`TriggerEvent()` returns `true` when a transition begins and `false` when it
is ignored or blocked.

1. If the machine is already transitioning, the event is ignored.
2. If the machine has not been started, the event is ignored.
3. The machine searches for a transition where:
   - `transition.from == current state`
   - `transition.event == event`
4. If no transition exists, nothing happens.
5. If a guard exists and returns false, nothing happens.
6. `WhenTransitionStarted` is called.
7. The transition `OnBefore` callback is called.
8. The current state's `OnExit` callback is called.
9. The target state's `OnEnter` callback is called.
10. If `OnEnter` succeeds, the current state is updated.
11. `WhenTransitionFinished` is called.
12. The transition `OnAfter` callback is called.
13. The transition is recorded in history.
14. The transitioning flag is cleared.

## Callback order

For a successful normal transition, the current tested order is:

1. `WhenTransitionStarted`
2. `OnBefore`
3. `OnExit`
4. `OnEnter`
5. `WhenTransitionFinished`
6. `OnAfter`

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

If the initial state has no `OnEnter`, startup completes immediately with
`started == true`, `current == initial`, and `transitioning == false`.

## Current limitations

- Events fired during a transition are ignored, not queued.
- `Start()` uses a synthetic `__start` history record instead of a normal transition event.
- Async callbacks capture the `StateMachine` object. The caller must ensure the `StateMachine` outlives pending callbacks.
- `GoBack()` uses a synthetic direct transition named `"__back"`.
- `GoBack()` does not require a registered reverse transition.
- Event queueing is not implemented.
- Transition cancellation is not implemented.
