# Changelog

## Unreleased

### Changed

- Reorganized repository into separate library, examples, tests, and docs areas.
- Moved reusable U++ package into `statemachine/`.
- Moved GUI test harness into `examples/StateMachineGuiTest/`.
- Hardened `Start()`, `TriggerEvent()`, and `TryTransition()` return values and state checks.
- Locked down callback ordering for successful transitions.
- Treat initial `OnEnter` as part of startup transition handling.
- Keep `OnAfter` bound to the exact transition object.
- Replaced ASSERT-only console tests with a reporting command-line test runner.
- Made `AddState()` and `AddTransition()` validated `bool` APIs.
- Made async completion callbacks single-shot.
- Made logging opt-in and quiet by default.

### Added

- Added non-GUI core test package under `tests/StateMachineCoreTest`.
- Added API, testing, roadmap, and design documentation under `docs/`.
- Added callback-order regression coverage.
- Added deterministic reporting test runner output.
- Added grouped startup, transition, failure, callback ordering, async, history, and stress tests.
- Added pasteable PASS/FAILED console output for review.
- Added minimal history inspection APIs for tests and diagnostics.
- Added configuration tests for state/transition validation and logging flags.

### Fixed

- Updated README paths and include examples.
- Fixed stale license wording.
- Fixed `OnAfter` lookup to use the exact transition object.
- Fixed `TriggerEvent()` to reject invalid registered transitions before returning success.
