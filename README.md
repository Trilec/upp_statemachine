# upp_statemachine

U++ state machine library with a GUI demo, visualizer, and core test package.

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
│   └── API.md
├── README.md
├── CHANGELOG.md
└── LICENSE
```

## Packages

- `statemachine/statemachine.upp` contains the reusable library package.
- `examples/StateMachineGuiTest/StateMachineGuiTest.upp` builds the GUI test harness.
- `tests/StateMachineCoreTest/StateMachineCoreTest.upp` builds the console core tests.

## Build

Use the repo-local `build/` directory for generated artifacts:

```powershell
& 'E:/upp-18468/umk.exe' 'E:/apps/github/upp_statemachine,E:/upp-18468/uppsrc' 'tests/StateMachineCoreTest' 'E:/upp-18468/CLANGx64.bm' --out-dir 'E:/apps/github/upp_statemachine/build' -abv
& 'E:/upp-18468/umk.exe' 'E:/apps/github/upp_statemachine,E:/upp-18468/uppsrc' 'examples/StateMachineGuiTest' 'E:/upp-18468/CLANGx64.bm' --out-dir 'E:/apps/github/upp_statemachine/build' -abv
& 'E:/upp-18468/umk.exe' 'E:/apps/github/upp_statemachine,E:/apps/github/upp_Ui,E:/apps/github/upp_AnimationEasing,E:/upp-18468/uppsrc' 'examples/StateMachineVisualizer' 'E:/upp-18468/CLANGx64.bm' --out-dir 'E:/apps/github/upp_statemachine/build' -abv
```

The current `.var` file points `OUTPUT` at `build/`, and the CLI `--out-dir`
switch keeps UMK from wandering off into the toolchain `out/` tree.
The finished executables are also mirrored at the top of `build/` for easier
finding:

- `build/StateMachineCoreTest.exe`
- `build/StateMachineGuiTest.exe`
- `build/StateMachineVisualizer.exe`

## Release Baseline

- Accepted `v1.0.1` baseline
- `StateMachineCoreTest`: `190/190`
- `StateMachineGuiTest`: builds
- `StateMachineVisualizer`: builds and launches
- `statemachine/statemachine.upp`: `Core` only

## Notes

- The library sources live in `statemachine/`.
- The demo and tests are split into separate U++ packages.
- Project documentation is now concentrated in `docs/`.
- `AddState()` and `AddTransition()` return `bool`.
- States must be added before transitions.
- Duplicate states and duplicate `from` + `event` transitions are rejected.
- `Start()` and `TriggerEvent()` return `bool`.
- `TriggerEvent()` validates source and target before it can return `true`.
- `EventPolicy` supports:
  - `RejectWhileTransitioning`
  - `DropWhileTransitioning`
  - `QueueWhileTransitioning` as a bounded FIFO `TriggerEvent()`-only queue
- Queue capacity failures report `EventQueueFull`.
- Drain-cycle protection reports `EventQueueDrainLimitReached`.
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
- Queueing is limited to queued `TriggerEvent()` event names only; queued `TryTransition()` and `GoBack()` are not supported.
- `StateMachineGuiTest` is a GUI example and manual visual harness, not the authoritative regression suite.
- `StateMachineVisualizer` is an optional animated visual/manual harness.
- Transition cancellation is not implemented.
- `true` usually means the operation was accepted or began, not that async work has finished.

## Quickstart

```cpp
#include <Core/Core.h>
#include <statemachine/statemachine.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
StateMachine sm;

if(!sm.AddState({"Idle", {}, {}}))
    return;

if(!sm.SetInitial("Idle"))
    return;

if(!sm.Start())
    LOG(sm.GetLastErrorText());

}
```

## Async Example

```cpp
#include <Core/Core.h>
#include <statemachine/statemachine.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
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

}
```

`true` means the operation was accepted or began. It does not imply that any
async `OnEnter` or `OnExit` work has already finished.

## License

Apache License 2.0
