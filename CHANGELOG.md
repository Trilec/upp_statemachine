# Changelog

## Unreleased

### Changed

- Reorganized repository into separate library, examples, tests, and docs areas.
- Moved reusable U++ package into `statemachine/`.
- Moved GUI test harness into `examples/StateMachineGuiTest/`.
- Hardened `Start()`, `TriggerEvent()`, and `TryTransition()` return values and state checks.
- Locked down callback ordering for successful transitions.

### Added

- Added non-GUI core test package under `tests/StateMachineCoreTest`.
- Added API, testing, roadmap, and design documentation under `docs/`.
- Added callback-order regression coverage.

### Fixed

- Updated README paths and include examples.
- Fixed stale license wording.
- Fixed `OnAfter` lookup to use the exact transition object.
