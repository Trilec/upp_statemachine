# upp-stateMachine: Design Specification

## Overview

`upp-stateMachine` is a lightweight, asynchronous, animation-aware, and U++-native finite state machine (FSM) framework designed to manage application UI and logic states in a declarative and robust way. It integrates closely with U++ features like `Animate()`, `PostCallback()`, and `AnimateProperty()`, and is intended as a reusable core for both small apps and large-scale modular U++ projects.

## Goals

- Event-driven state transitions
- Asynchronous transition control (e.g., for animations)
- Clean separation of state logic from UI layout
- Support for state-driven view switching
- Header-first, lightweight implementation with optional .cpp file
- Designed for future expansion (e.g., hierarchical states)

## Core Concepts

### 1. Event-Driven Transitions

Rather than directly targeting states, transitions are triggered by **events**:

```cpp
sm.TriggerEvent("user_logged_in");
```

This improves decoupling between UI logic and state logic, allowing states to decide how to handle each event.

### 2. State Definition

```cpp
struct State {
    String id;
    Function<void(Function<void()> done)> OnEnter; // Required to call done() to finalize entry
    Function<void(Function<void()> done)> OnExit;  // Optional exit logic (also requires done if async)
};
```

Entry and exit logic can be asynchronous. Transitions wait for `done()` before continuing.

### 3. Transition Definition

```cpp
struct TransitionContext {
    StateMachine& machine;
    const String& fromState;
    const String& toState;
    const String& event;
};

struct Transition {
    String event;
    String from;
    String to;
    Function<void(TransitionContext&)> OnBefore; // Optional hook before transition
    Function<void(TransitionContext&)> OnAfter;  // Optional hook after transition
    Function<bool(TransitionContext&)> Guard;    // Optional guard condition
};
```

Guards can block transitions. Hooks provide transition metadata for debug or logic injection.

### 4. Transition Record (for History)

```cpp
struct TransitionRecord {
    String from;
    String to;
    String event;
};
```

These are tracked in `transitionHistory` to allow full undo support and backtracking.

### 5. StateMachine Class

```cpp
class StateMachine {
public:
    void AddState(const State& s);
    void AddTransition(const Transition& t);
    void SetInitial(const String& id);
    void Start();
    void TriggerEvent(const String& event);
    bool TryTransition(const Transition& t);

    String GetCurrent() const;
    bool IsTransitioning() const;
    bool CanGoBack() const;
    void GoBack();

    Function<void(const TransitionContext&)> WhenTransitionStarted;
    Function<void(const TransitionContext&)> WhenTransitionFinished;

private:
    Vector<TransitionRecord> transitionHistory;
    Vector<State> states;
    Vector<Transition> transitions;
    String current, initial;
    bool transitioning = false;
};
```

### 6. Transition Workflow

```text
TriggerEvent("event")
→ Check if valid transition exists for current state
→ If valid and guard passes:
    → Call OnExit(done) of current state
    → Call OnBefore()
    → Call OnEnter(done) of new state
    → Call OnAfter()
    → Update current state and push TransitionRecord to history
```

### 7. Asynchronous Animation Integration

```cpp
sm.AddState({"FadeIn", [&](auto done) {
    Animate(panel).Time(300).Easing(Easing::EaseOutQuad).Opacity(1.0).Then(done);
}});
```

The `done()` callback ensures transitions don’t interrupt running animations.

### 8. StateViewManager (Optional)

```cpp
StateViewManager mgr(&sm, &container);
mgr.RegisterView("StateA", &viewA);
mgr.RegisterView("StateB", &viewB);
mgr.EnableAnimation(true);
mgr.SetFadeTime(250);
```

This utility auto-manages `Ctrl` visibility based on FSM state changes, supporting fade animations.

### 9. Reliable GoBack() Mechanism

`GoBack()` uses complete transition records to enable robust state reversal. The workflow:

1. FSM tracks every successful transition as a `TransitionRecord`.
2. `GoBack()` pops the last record.
3. It attempts to find a reverse transition (`from=to`, `to=from`).
4. If none exists, it may optionally force a direct state switch.
5. Transitions are only attempted if no animation is already in progress.

This guarantees:
- Proper `OnExit` and `OnEnter` calls
- Optional reapplication of guard logic or hooks
- No overlapping transitions

## Example Usage

```cpp
StateMachine sm;
sm.SetInitial("Idle");

sm.AddState({"Idle",  [](auto done){ LOG("Idle"); done(); }, {}});
sm.AddState({"Working", [](auto done){ Animate(panel).Then(done); }, {}});

sm.AddTransition({
    .event = "start",
    .from = "Idle",
    .to = "Working",
    .Guard = [](auto ctx){ return true; },
    .OnBefore = [](auto ctx){ LOG("Starting..."); },
    .OnAfter = [](auto ctx){ LOG("Now working"); }
});

sm.Start();
sm.TriggerEvent("start");
// Later...
sm.GoBack();
```

## Planned Roadmap Features

- ✅ Event-based transitions
- ✅ Async transitions with animation support
- ✅ TransitionContext for full metadata access
- ✅ View manager for Ctrl visibility and animation
- ✅ State history & `GoBack()`
- ✅ Transition logging hooks
- 🕘 Hierarchical states (HSMs)
- 🕘 Code generation template (macro-free)
- 🕘 Queued transitions & cancellation safety

## License & Structure

- MIT or BSD
- Project: `upp-stateMachine`
- Nested under `upp-` namespace and installable via U++ package system

```
upp-stateMachine/
├── include/
│   └── StateMachine.h
├── src/
│   └── StateMachine.cpp
├── design/
│   └── design.md
├── LICENSE
├── README.md
├── stateMachine.upp
```
