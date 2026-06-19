# upp_statemachine

U++ state machine library with a GUI demo and a core test package.

## Layout

```text
upp_statemachine/
├── statemachine/
│   ├── statemachine.upp
│   ├── statemachine.h
│   └── statemachine.cpp
├── examples/
│   └── StateMachineGuiTest/
│       ├── StateMachineGuiTest.upp
│       └── main.cpp
├── tests/
│   └── StateMachineCoreTest/
│       ├── StateMachineCoreTest.upp
│       └── main.cpp
├── docs/
│   ├── DESIGN.md
│   ├── API.md
│   ├── TESTING.md
│   └── ROADMAP.md
├── README.md
├── CHANGELOG.md
└── LICENSE
```

## Packages

- `statemachine/statemachine.upp` contains the reusable library package.
- `examples/StateMachineGuiTest/StateMachineGuiTest.upp` builds the GUI test harness.
- `tests/StateMachineCoreTest/StateMachineCoreTest.upp` builds the console core tests.

## Notes

- The library sources live in `statemachine/`.
- The demo and tests are split into separate U++ packages.
- Project documentation is now concentrated in `docs/`.
- `AddState()` and `AddTransition()` return `bool`.
- States must be added before transitions.
- Duplicate states and duplicate `from` + `event` transitions are rejected.
- `Start()` and `TriggerEvent()` return `bool`.
- `TriggerEvent()` validates source and target before it can return `true`.
- `IsStarted()` reports whether startup has been accepted and the machine owns a current initial state.
- `Start()` treats the initial `OnEnter` as a transition phase.
- Async completion callbacks are single-shot.
- `TryTransition()` requires `t.from == current`.
- History inspection helpers are available for tests and diagnostics.
- Logging is disabled by default.
- `OnAfter` uses the exact transition object passed in.
- Event queueing and transition cancellation are not implemented yet.

## License

Apache License 2.0
