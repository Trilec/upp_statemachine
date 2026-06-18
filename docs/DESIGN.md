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

- `Start()` returns `bool`.
- `TriggerEvent()` returns `bool`.
- `IsStarted()` indicates whether startup completed successfully.
- `TryTransition()` requires `t.from == current`.
- `OnAfter` runs from the exact transition object passed into `DoTransition()`.
- Event queueing is not implemented.
- Transition cancellation is not implemented.
