Here is the updated `README.md` draft for the GitHub repository, aligned with the finalized header and design document:

---

# upp-stateMachine

A lightweight, asynchronous, animation-aware finite state machine (FSM) framework designed for [U++ (Ultimate++)](https://www.ultimatepp.org/) applications. `upp-stateMachine` cleanly separates UI control logic from visual layout, supports robust state transitions, and integrates seamlessly with U++ features like `Animate()` and `PostCallback()`.

## ✨ Features

* 🧠 **Event-driven** declarative state machine
* 🎞 **Async transitions** with animation support
* 🧩 **Header-first** design with optional `.cpp` backend
* 🧭 **State history + GoBack()** tracking
* 🧱 Optional **StateViewManager** for automated view switching
* 🧼 Zero third-party dependencies, fully U++-native

---

## 📦 File Layout

```
upp-stateMachine/
├── include/
│   └── StateMachine.h       # Main header (public API)
├── src/
│   └── StateMachine.cpp     # Optional .cpp for logic (used by default)
├── design/
│   └── design.md            # Full architecture and concept doc
├── stateMachine.upp         # U++ package metadata
├── README.md
├── LICENSE                  # MIT or BSD
```

---

## 🚀 Getting Started

1. Add the `upp-stateMachine` package to your U++ project.
2. Include the header:

```cpp
#include <StateMachine/StateMachine.h>
```

3. Define states and transitions:

```cpp
using namespace Upp;

StateMachine sm;

sm.SetInitial("Idle");

sm.AddState({
    "Idle",
    [](auto done) { LOG("Now idle."); done(); },
    [](auto done) { LOG("Leaving idle."); done(); }
});

sm.AddState({
    "Working",
    [](auto done) { Animate(panel).Opacity(1).Time(250).Then(done); },
    [](auto done) { Animate(panel).Opacity(0).Time(250).Then(done); }
});

sm.AddTransition({
    .event = "start",
    .from = "Idle",
    .to = "Working",
    .Guard = [](auto) { return true; },
    .OnBefore = [](auto) { LOG("Transitioning..."); },
    .OnAfter = [](auto) { LOG("Entered Working."); }
});

sm.Start();
sm.TriggerEvent("start");
```

---

## 🧭 StateViewManager (optional)

```cpp
StateViewManager mgr(&sm, &container);
mgr.RegisterView("Idle", &idlePanel);
mgr.RegisterView("Working", &workingPanel);
mgr.EnableAnimation(true);
mgr.SetFadeTime(300);
```

---

## ⏪ GoBack()

The state machine maintains a full transition history. Calling `GoBack()` will undo the last transition cleanly, triggering all exit/enter logic. Works asynchronously and safely ignores calls while a transition is still in progress.

---

## 🛠 Planned Features

* [x] Event-driven transitions
* [x] Asynchronous `done()`-based animation flow
* [x] Transition context + hooks
* [x] Auto view manager
* [x] Full history tracking
* [ ] Hierarchical states (HSMs)
* [ ] Queued transitions
* [ ] Cancel-safe logic
* [ ] Code generation utilities

---

## 📄 License

Released under the MIT or BSD license. See [LICENSE](./LICENSE).

---

Would you like this saved directly into your project folder or uploaded to the canvas alongside the `.cpp` and `.upp` files?
