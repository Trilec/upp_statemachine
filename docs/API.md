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

- `Start()`
- `TriggerEvent(const String& e)`
- `TryTransition(const Transition& t)`
- `GoBack()`

### Queries

- `GetCurrent() const`
- `IsTransitioning() const`
- `CanGoBack() const`

### Hooks

- `WhenTransitionStarted`
- `WhenTransitionFinished`

## TriggerEvent(event)

1. If the machine is already transitioning, the event is ignored.
2. The machine searches for a transition where:
   - `transition.from == current state`
   - `transition.event == event`
3. If no transition exists, nothing happens.
4. If a guard exists and returns false, nothing happens.
5. `WhenTransitionStarted` is called.
6. The transition `OnBefore` callback is called.
7. The current state's `OnExit` callback is called.
8. The target state's `OnEnter` callback is called.
9. If `OnEnter` succeeds, the current state is updated.
10. `WhenTransitionFinished` is called.
11. The transition `OnAfter` callback is called.
12. The transition is recorded in history.
13. The transitioning flag is cleared.

## Current limitations

- Events fired during a transition are ignored, not queued.
- `Start()` does not treat the initial `OnEnter` as a transition.
- Async callbacks capture the `StateMachine` object. The caller must ensure the `StateMachine` outlives pending callbacks.
- `GoBack()` uses a synthetic direct transition named `"__back"`.
- `GoBack()` does not require a registered reverse transition.
