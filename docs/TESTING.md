# Testing

## Test packages

- `tests/StateMachineCoreTest` is the non-GUI core test package.
- `examples/StateMachineGuiTest` is the GUI test harness.

## Required checks

Build and run:

1. `tests/StateMachineCoreTest`
2. `examples/StateMachineGuiTest`

The core test must not depend on GUI packages.
The GUI test may depend on CtrlLib and RichEdit.
