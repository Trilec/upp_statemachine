# StateMachineVisualizer — Gary Notes

## Current goal

This package is a first visual scaffold for a high-end StateMachine GUI tester inspired by Curt's supplied HTML mockup.

It is **not** the authoritative regression suite. `tests/StateMachineCoreTest` remains the authority and should stay green at the current accepted baseline.

## Important boundaries

Do not change:

- `statemachine/statemachine.upp`
- the StateMachine public API
- the core package dependency model

The core package must remain `Core` only.

This visualizer is optional and may depend on UI/GUI packages.

## Intended architecture

- `StateNodeCard : UiTitleCard`
  - visual state node
  - themed card surface
  - active / queued / error / completed indication
  - no ownership of the StateMachine core

- `GraphView : Ctrl`
  - owns grid placement of node cards
  - paints subtle background grid
  - paints curved edges behind node controls
  - paints animated tokens moving along edges

- `VisualizerModel`
  - view-model only
  - stores node specs, edge specs, tokens, and log rows

- `VisualizerApp : TopWindow`
  - owns the shell, control bar, graph, log, guide panel, and real `StateMachine`

## First compile expectations

This is an architecture scaffold, not final polished code. Expect to do a normal compile-fix pass.

Likely areas to check:

1. Package assembly/nests
   - The build assembly must include:
     - `E:/apps/github/upp_statemachine`
     - `E:/apps/github/upp_Ui`
     - `E:/apps/github/upp_AnimationEasing`
     - `E:/upp-18468/uppsrc`
   - `Ui` depends on the external Animation package from `upp_AnimationEasing`.

2. U++ API nits
   - If a helper like `Blend`, `clamp`, `DrawEllipse(Rect, Color)`, or `Ctrl::GlobalBackPaint()` differs in this local U++ build, adjust locally without changing the architecture.
   - Prefer small compile fixes over rewriting the structure.

3. UiTitleCard styling
   - `StateNodeCard` intentionally uses `UiTitleCard::SetCustomStyle()`.
   - If any style field names have drifted, copy the current pattern from `upp_Ui/examples/UiTitleCardDemo/main.cpp`.

4. Animation
   - The first scaffold uses a timer tick for token progress.
   - Once it compiles, replace or supplement token progress with the Animation package if that is the cleaner local pattern.
   - Keep the visible behavior: moving tokens along curved edges.

## Visual goals

First milestone:

- dark shell
- top header
- controls row
- graph canvas with subtle grid
- grid-placed `StateNodeCard` nodes
- curved edges
- moving token dots
- footer log and guide panel

Second milestone:

- bind more buttons to real StateMachine scenarios:
  - reject while transitioning
  - drop while transitioning
  - queue while transitioning
  - queue full
  - drain limit reached
  - failed startup
  - failed OnExit
  - failed OnEnter
  - GoBack

Third milestone:

- add a light theme option or theme toggle
- add better active indication
- improve node spacing and labels
- add pan/zoom if needed

## Acceptance for this scaffold

- `StateMachineVisualizer` builds.
- `StateMachineGuiTest` still builds.
- `StateMachineCoreTest` still passes.
- `statemachine/statemachine.upp` remains Core-only.

## Current known limitation

The first scaffold only approximates the full mockup. It sets up the right architecture so the visual polish and deeper scenarios can be added without contaminating the StateMachine core.
