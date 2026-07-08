# Changelog

## v1.0.1

Initial compact U++ state machine release.

### Changed

- Reorganized repository into separate library, examples, tests, and docs areas.
- Moved reusable U++ package into `statemachine/`.
- Moved GUI test harness into `examples/StateMachineGuiTest/`.
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
- Treat initial `OnEnter` as part of startup transition handling.
- Cleared queued events on failed async startup while preserving `StartEnterFailed` rollback behavior.
- Keep `OnAfter` bound to the exact transition object.
- Replaced ASSERT-only console tests with a reporting command-line test runner.
- Made `AddState()` and `AddTransition()` validated `bool` APIs.
- Made async completion callbacks single-shot.
- Made logging opt-in and quiet by default.

### Added

- Added non-GUI core test package under `tests/StateMachineCoreTest`.
- Added API, testing, roadmap, and design documentation under `docs/`.
- Added callback-order regression coverage.
- Added callback-phase state snapshot coverage for transition lifecycle callbacks.
- Added deterministic reporting test runner output.
- Added grouped startup, transition, failure, callback ordering, async, history, and stress tests.
- Added bounded queue-policy coverage, including startup and `GoBack()` queue ordering cases.
- Added drain-cycle protection coverage for self-feeding queued-event loops.
- Added pasteable PASS/FAILED console output for review.
- Added minimal history inspection APIs for tests and diagnostics.
- Added configuration tests for state/transition validation and logging flags.
- Added error API coverage for rejected operations and cleared-success behavior.
- Added `GoBack()` return-value coverage.
- Added `Reset()` and `Clear()` lifecycle coverage.
- Added query-helper coverage for existence checks and count tracking.

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
