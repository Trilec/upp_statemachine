# Testing

## Test packages

- `tests/StateMachineCoreTest` is the non-GUI core test package.
- `examples/StateMachineGuiTest` is the GUI test harness.

## Command line workflow

1. Build `tests/StateMachineCoreTest` with your local U++ command-line build tool.
2. Run the generated console executable from the shell.
3. Paste the full console output into review when validating changes.

The exact command depends on your U++ setup, but the core rule is simple: build the core package first, then execute it directly.

## Required checks

1. Build and run `tests/StateMachineCoreTest`.
2. Build and run `examples/StateMachineGuiTest`.

The core test suite must be deterministic and non-GUI.
The GUI test is secondary and should not be required for core validation.
It can depend on CtrlLib and RichEdit, but the console suite should stay cleanly separated.

## Expected output

```text
== Startup ==
Start valid initial state: PASSED
Start empty initial state rejected: PASSED

== Summary ==
Passed: 42
Failed: 0
Total: 42
ALL TESTS PASSED
```

## Coverage notes

- `tests/StateMachineCoreTest` covers startup, transitions, history, callback ordering, async completion, and stress cases.
- `Start()` is tested as a transition phase for initial entry.
- `TryTransition()` is tested with the current-state check.
- `OnAfter` is tested against the exact transition object.
- Event queueing and transition cancellation remain unimplemented and should not be tested as existing behavior.
