# Design

`upp_statemachine` is a small U++-native finite state machine library designed
to orchestrate application logic and UI state changes.

## Package paths

- `statemachine/statemachine.h`
- `statemachine/statemachine.cpp`
- `statemachine/statemachine.upp`

## Goals

- event-driven transitions
- asynchronous enter and exit handlers
- transition guards and hooks
- history tracking with `GoBack()`
- a reusable core package with no GUI dependency

## Core model

- `State` holds the state id plus optional `OnEnter` and `OnExit` handlers.
- `Transition` links one state to another through an event.
- `TransitionContext` carries the active machine, source state, target state,
  and triggering event into callbacks.
- `TransitionRecord` stores completed transitions for history.
- `StateMachine` owns the states, transitions, and history stack.

## Transition flow

1. The machine starts in the configured initial state.
2. `Start()` treats the initial `OnEnter` as a transition phase.
3. `TriggerEvent()` finds the matching transition for the current state.
4. The transition guard runs, if present.
5. The current state exits, then the target state enters.
6. Transition hooks run around the state callbacks.
7. The completed transition is recorded in history.

## History

`GoBack()` uses the recorded transition history to move back to the previous
state without exposing the caller to the internal transition machinery.

## Package split

- `statemachine/` contains the reusable library package for `upp_statemachine`.
- `examples/StateMachineGuiTest/` contains the graphical test harness.
- `tests/StateMachineCoreTest/` contains the non-GUI core tests.

## Current hardening notes

- `AddState()` returns `bool`.
- `AddState()` rejects empty ids, duplicate ids, and late additions after start.
- `AddTransition()` returns `bool`.
- `AddTransition()` rejects empty fields, missing endpoint states, duplicate `from` + `event` pairs, and late additions after start.
- `Start()` returns `bool`.
- `TriggerEvent()` returns `bool`.
- `GoBack()` returns `bool`.
- `Reset()` returns `bool` and keeps configuration.
- `Clear()` returns `bool` and clears configuration.
- Query helpers expose read-only counts and existence checks.
- `TriggerEvent()` validates source and target before it can succeed.
- `GetLastError()` and `GetLastErrorText()` expose the last public failure.
- `IsStarted()` means `Start()` has been accepted and the machine owns a current initial state.
- During async initial `OnEnter`, `IsStarted()` and `IsTransitioning()` are both `true`.
- If initial `OnEnter` later fails, startup rolls back and `IsStarted()` becomes `false`.
- Machine-owned completion callbacks are single-shot.
- History inspection helpers expose the recorded entries for tests and diagnostics.
- Logging is opt-in; normal transition flow is quiet unless enabled.
- `TryTransition()` requires `t.from == current`.
- `OnAfter` runs from the exact transition object passed into `DoTransition()`.
- `EventPolicy` is stored on the machine, but queueing is not implemented yet.
- Current implemented behavior:
  - `RejectWhileTransitioning`
  - `DropWhileTransitioning`
- Declared but not fully implemented:
  - `QueueWhileTransitioning`
- `TriggerEvent()` while transitioning reports:
  - `EventRejectedWhileTransitioning`
  - `EventDroppedWhileTransitioning`
  - `EventQueueingNotImplemented`
- Event queueing is not implemented.
- Transition cancellation is not implemented.
- In this API, `true` usually means the operation was accepted or began; it does not imply async completion.
