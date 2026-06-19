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
- `Start()` and `TriggerEvent()` return `bool`.
- `IsStarted()` reports whether startup has completed successfully.
- `Start()` treats the initial `OnEnter` as a transition phase.
- `TryTransition()` requires `t.from == current`.
- `OnAfter` uses the exact transition object passed in.
- Event queueing and transition cancellation are not implemented yet.

## License

Apache License 2.0
