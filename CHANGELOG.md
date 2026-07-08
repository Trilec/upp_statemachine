# Changelog

## Unreleased

### Added

- Added the initial `examples/StateMachineVisualizer` scaffold for a richer animated/manual GUI harness using `Ui`, `Painter`, and `Animation`.

The visualizer is experimental and is not part of the accepted v0.1.0 verification baseline until its compile and visual pass are complete.

## v0.1.0

Initial compact U++ state machine release.

### Changed

- Reorganized the repository into separate library, examples, tests, and docs areas.
- Moved the reusable U++ package into `statemachine/`.
- Moved the lightweight GUI test harness into `examples/StateMachineGuiTest/`.
- Finalized `EventPolicy` behavior for reject, drop, and bounded FIFO queued `TriggerEvent()` names while transitioning.
- Added a bounded synchronous queue drain-cycle limit using `max_queued_events`.
- Hardened `Start()`, `TriggerEvent()`, and `TryTransition()` return values and state checks.
- Committed transition history before `WhenTransitionFinished` and `OnAfter` so completion callbacks observe the finalized transition.
- Added lightweight error reporting via `GetLastError()` and `GetLastErrorText()`.
- Made `GoBack()` return `bool`.
- Added `Reset()` for runtime-only reset and `Clear()` for full teardown.
- Added read-only query helpers for states, transitions, and counts.
- Added queued-event inspection and control helpers.
- Locked down callback ordering for successful transitions.
- Treated initial `OnEnter` as part of startup transition handling.
- Cleared queued events on failed async startup while preserving `StartEnterFailed` rollback behavior.
- Kept `OnAfter` bound to the exact transition object.
- Replaced ASSERT-only console tests with a reporting command-line test runner.
- Made `AddState()` and `AddTransition()` validated `bool` APIs.
- Made machine-owned async completion callbacks single-shot.
- Made logging opt-in and quiet by default.

### Added

- Added the non-GUI core test package under `tests/StateMachineCoreTest`.
- Added focused API and design documentation under `docs/`.
- Added callback-order and callback-phase state regression coverage.
- Added deterministic reporting test output.
- Added grouped startup, transition, failure, callback ordering, async, history, lifecycle, queue-policy, and stress coverage.
- Added bounded queue-policy coverage, including startup and `GoBack()` ordering cases.
- Added drain-cycle protection coverage for self-feeding queued-event loops.
- Added minimal history inspection APIs for tests and diagnostics.
- Added configuration, error API, `GoBack()`, `Reset()`, `Clear()`, and query-helper coverage.

### Verified

- `StateMachineCoreTest` is green at `190/190`.
- `StateMachineGuiTest` builds successfully.
- `statemachine/statemachine.upp` depends only on `Core`.

### Fixed

- Updated README paths and include examples.
- Fixed stale license wording.
- Fixed `OnAfter` lookup to use the exact transition object.
- Fixed `TriggerEvent()` to reject invalid registered transitions before returning success.
- Fixed queue draining after successful async `Start()`.
- Fixed queue draining after successful `GoBack()` so history pop completes first.
- Fixed self-feeding synchronous queued-event loops so they stop after the configured drain limit and report `EventQueueDrainLimitReached` without corrupting state or history.
