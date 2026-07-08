# upp_statemachine

A compact asynchronous finite state machine for Ultimate++.

The reusable package is GUI-independent and depends only on `Core`. It supports
validated state/transition setup, asynchronous enter/exit callbacks, guards,
transition hooks, history with `GoBack()`, reset/reuse, lightweight error
reporting, and bounded event handling while a transition is active.

## Status

Accepted v1.0.1 baseline:

- `StateMachineCoreTest`: `190/190`
- `StateMachineGuiTest`: builds successfully
- `StateMachineVisualizer`: builds and launches
- `statemachine/statemachine.upp`: depends only on `Core`

The newer `StateMachineVisualizer` package is an optional animated
manufacturing-flow visual/manual harness using `Ui`, `Painter`, and
`Animation`.

## Layout

```text
upp_statemachine/
├── statemachine/
│   ├── statemachine.upp
│   ├── statemachine.h
│   └── statemachine.cpp
├── examples/
│   ├── StateMachineGuiTest/
│   │   ├── StateMachineGuiTest.upp
│   │   └── main.cpp
│   └── StateMachineVisualizer/
│       ├── StateMachineVisualizer.upp
│       ├── VisualizerModel.h
│       ├── StateNodeCard.h/.cpp
│       ├── GraphView.h/.cpp
│       ├── VisualizerApp.h/.cpp
│       └── main.cpp
├── tests/
│   └── StateMachineCoreTest/
│       ├── StateMachineCoreTest.upp
│       └── main.cpp
├── docs/
│   ├── API.md
│   └── DESIGN.md
├── README.md
├── CHANGELOG.md
└── LICENSE
```

## Packages

- `statemachine/statemachine.upp` — reusable Core-only library package.
- `tests/StateMachineCoreTest/StateMachineCoreTest.upp` — authoritative non-GUI regression suite.
- `examples/StateMachineGuiTest/StateMachineGuiTest.upp` — lightweight manual GUI harness and GUI build check.
- `examples/StateMachineVisualizer/StateMachineVisualizer.upp` — optional animated manufacturing-flow visual/manual harness using `Ui`, `Painter`, and `Animation`.

## Core behavior

- `AddState()` and `AddTransition()` validate input and return `bool`.
- States must be added before transitions.
- Duplicate states and duplicate `from` + `event` transition keys are rejected.
- `Start()`, `TriggerEvent()`, `TryTransition()`, `GoBack()`, `Reset()`, and `Clear()` return `bool`.
- `true` generally means an operation was accepted or began; asynchronous work may still be pending.
- `GetLastError()` and `GetLastErrorText()` expose the last public failure.
- `Reset()` clears runtime state while preserving configuration.
- `Clear()` clears runtime state and configuration.
- Logging is opt-in and quiet by default.

### Event handling while transitioning

`EventPolicy` supports:

- `RejectWhileTransitioning`
- `DropWhileTransitioning`
- `QueueWhileTransitioning`

Queueing is intentionally limited to bounded FIFO `TriggerEvent()` event names.
Queued `TryTransition()` and `GoBack()` operations are not supported.

- queue capacity failure: `EventQueueFull`
- synchronous drain-cycle protection: `EventQueueDrainLimitReached`

The drain-cycle limit prevents self-feeding synchronous event chains from
running indefinitely while preserving remaining queued events and valid machine
state.

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
    if(!sm.AddState({"Working", {}, {}}))
        return;
    if(!sm.AddTransition({"start", "Idle", "Working"}))
        return;
    if(!sm.SetInitial("Idle"))
        return;
    if(!sm.Start()) {
        LOG(sm.GetLastErrorText());
        return;
    }
    if(!sm.TriggerEvent("start"))
        LOG(sm.GetLastErrorText());
}
```

## Asynchronous callbacks

`OnEnter` and `OnExit` receive a single-shot `done(bool)` completion callback.
A successful return from `Start()` or `TriggerEvent()` means the operation was
accepted; it does not mean asynchronous work has already completed.

The current implementation assumes same-thread, same-callback-chain use and
does not provide internal locking. The `StateMachine` object must outlive any
pending asynchronous completion callback.

## Documentation

- [`docs/API.md`](docs/API.md) — public API and exact behavioral contract.
- [`docs/DESIGN.md`](docs/DESIGN.md) — architecture, lifecycle, boundaries, and future directions.
- [`CHANGELOG.md`](CHANGELOG.md) — v0.1.0 release details.

## Current boundaries

- flat states only; hierarchical states are not implemented
- transition cancellation is not implemented
- queueing is event-name-only and single-threaded
- the GUI examples are optional and do not add GUI dependencies to the core package

## Windows build note

`GitHubOut.var` contains the maintainer's working Windows/U++ assembly.
`GitHubOut.var.example` is the sanitized template for other workspaces.
Generated files belong under `build/`.

## License

Apache License 2.0.
