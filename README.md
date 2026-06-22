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
│   ├── ROADMAP.md
│   └── RELEASE_CHECKLIST.md
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
- `EventPolicy` supports reject and drop behavior now; queue is declared but not fully implemented.
- `TriggerEvent()` while transitioning reports policy-specific errors.
- `IsStarted()` reports whether startup has been accepted and the machine owns a current initial state.
- `Start()` treats the initial `OnEnter` as a transition phase.
- Async completion callbacks are single-shot.
- `TryTransition()` requires `t.from == current`.
- `GetLastError()` and `GetLastErrorText()` report the last public failure.
- `GoBack()` returns `bool`.
- `Reset()` keeps configuration and clears runtime state.
- `Clear()` clears configuration and runtime state.
- Query helpers expose read-only existence checks and counts.
- History inspection helpers are available for tests and diagnostics.
- Logging is disabled by default.
- `OnAfter` uses the exact transition object passed in.
- Event queueing and transition cancellation are not implemented yet.
- `true` usually means the operation was accepted or began, not that async work has finished.

## Minimal Example

```cpp
StateMachine sm;

if(!sm.AddState({"Idle", {}, {}}))
    return;

if(!sm.SetInitial("Idle"))
    return;

if(!sm.Start())
    LOG(sm.GetLastErrorText());
```

## Async Example

```cpp
StateMachine sm;

if(!sm.AddState({"Idle", {}, {}}))
    return;
if(!sm.AddState({"Working",
    [&](StateMachine&, Function<void(bool)> done) {
        // show work, then finish later
        done(true);
    },
    {}
}))
    return;
if(!sm.AddTransition({"start", "Idle", "Working"}))
    return;
if(!sm.SetInitial("Idle"))
    return;
if(!sm.Start())
    LOG(sm.GetLastErrorText());
if(!sm.TriggerEvent("start"))
    LOG(sm.GetLastErrorText());
```

## License

Apache License 2.0
