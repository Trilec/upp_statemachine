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
- `StateMachine` owns the states, transitions, history stack, and bounded queued
  event-name list.

## Transition flow

1. The machine starts in the configured initial state.
2. `Start()` treats the initial `OnEnter` as a transition phase.
3. `TriggerEvent()` finds the matching transition for the current state.
4. The transition guard runs, if present.
5. The current state exits, then the target state enters.
6. The completed transition is recorded in history.
7. Transition hooks run around the state callbacks.
8. Successful completion may drain queued event names according to the active
   event policy and drain-cycle limit.

Observed state during a successful normal transition:

- `Guard`: current state is the source state; transition is not yet active.
- `WhenTransitionStarted` and `OnBefore`: current state is still the source
  state and `IsTransitioning()` is `true`.
- `OnExit`: current state is still the source state.
- `OnEnter`: current state is still the source state while the target entry
  callback runs.
- If `OnEnter` later fails, the machine rolls back to the source state, does
  not commit history, clears transitioning, and does not run the finished/after
  hooks.
- `WhenTransitionFinished` and `OnAfter`: current state is already the target
  state and the history entry is committed.
- After the callback chain unwinds, `IsTransitioning()` becomes `false`.

## History

`GoBack()` uses recorded transition history to move back to the previous state
without requiring a registered reverse transition.

## Package split

- `statemachine/` contains the reusable Core-only library package.
- `tests/StateMachineCoreTest/` contains the authoritative non-GUI regression suite.
- `examples/StateMachineGuiTest/` contains the lightweight graphical/manual harness.
- `examples/StateMachineVisualizer/` contains an optional animated manufacturing-flow visual/manual harness using `Ui`, `Painter`, and `Animation`.

The optional GUI packages do not change the dependency model of the reusable
core package. The visualizer stays outside the core API surface and does not
alter the reusable library contract.

## Current hardening notes

- `AddState()` rejects empty ids, duplicate ids, and late additions after start.
- `AddTransition()` rejects empty fields, missing endpoint states, duplicate
  `from` + `event` pairs, and late additions after start.
- `Start()`, `TriggerEvent()`, `TryTransition()`, `GoBack()`, `Reset()`, and
  `Clear()` return `bool`.
- Query helpers expose read-only counts and existence checks.
- `TriggerEvent()` validates source and target before it can succeed.
- `GetLastError()` and `GetLastErrorText()` expose the last public failure.
- `IsStarted()` means `Start()` has been accepted and the machine owns a current
  initial state.
- During async initial `OnEnter`, `IsStarted()` and `IsTransitioning()` are both
  `true`.
- If initial `OnEnter` later fails, startup rolls back, clears queued startup
  work, and leaves `StartEnterFailed` as the final error.
- Machine-owned completion callbacks are single-shot.
- The implementation assumes same-thread, same-callback-chain use and provides
  no internal locking.
- Pending async callbacks hold a live reference to the machine, so the
  `StateMachine` object must outlive them.
- History inspection helpers expose recorded entries for tests and diagnostics.
- Logging is opt-in; normal transition flow is quiet unless enabled.
- `TryTransition()` requires `t.from == current`.
- `OnAfter` runs from the exact transition object passed into `DoTransition()`.
- Successful completion callbacks observe the target current state and committed
  history while `IsTransitioning()` remains `true` until the callback chain
  unwinds.

## Event policy and queueing

`EventPolicy` controls only `TriggerEvent()` calls that arrive while a
transition is active:

- `RejectWhileTransitioning`
- `DropWhileTransitioning`
- `QueueWhileTransitioning`

Queueing uses a bounded FIFO list of event names only.

- queue capacity failures report `EventQueueFull`
- queued events drain only after successful completion and after
  `transitioning` is cleared
- failed active transitions leave the queue intact, except failed startup clears
  queued events during rollback
- a failed queued event is removed, preserves its failure error, and stops the
  remaining drain
- self-feeding synchronous drain chains are bounded by `max_queued_events` per
  drain cycle
- `EventQueueDrainLimitReached` stops an accidental infinite synchronous drain
  while preserving remaining queued events and valid runtime state

## Current boundaries

- flat states only
- no transition cancellation
- no internal thread synchronization
- queued `TryTransition()` and `GoBack()` are not supported
- `true` generally means an operation was accepted or began; it does not imply
  asynchronous completion

## Future directions

- cancellation policy
- hierarchical states
- richer GUI/state-view helpers after the visualizer scaffold is compiled and validated
- code-generation helpers
- UppHub packaging notes
