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
Passed: N
Failed: 0
Total: N
ALL TESTS PASSED
```

## Coverage notes

- `tests/StateMachineCoreTest` covers startup, transitions, history, callback ordering, async completion, and stress cases.
- The console suite also covers the lightweight error API and `GoBack()` return values.
- The console suite also covers `Reset()` and `Clear()` lifecycle control.
- The console suite also covers read-only query helpers and count reporting.
- Async completion callbacks are tested as single-shot.
- `Start()` is tested as a transition phase for initial entry.
- `TryTransition()` is tested with the current-state check.
- `OnAfter` is tested against the exact transition object.
- Logging is expected to stay quiet by default in the console suite.
- Event queueing and transition cancellation remain unimplemented and should not be tested as existing behavior.
- Boolean-returning API calls should be read as "accepted/began" unless the docs say otherwise.
