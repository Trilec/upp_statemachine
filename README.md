```markdown
# upp-stateMachine

A lightweight, asynchronous, animation-aware finite state machine (FSM) framework for Ultimate++ (U++).  
Cleanly separates UI control logic from visual layout, supports robust state transitions, and integrates seamlessly with U++'s event-driven model.

## ✨ Features

-  Event-driven, declarative API  
-  Async transitions with animation callbacks  
-  Full transition history + `GoBack()` support  
-  Header-only by default, zero third-party dependencies  

## 📦 File Layout

upp-stateMachine/
├── include/
│   └── StateMachine.h       # Main header (public API)
├── src/
│   └── StateMachine.cpp     # Optional .cpp for core logic
├── examples/
│   └── main.cpp             # GUI test harness (LogView + tests)
│   └── statemachine.upp     # U++ package metadata for the example
├── design/
│   └── design.md            # Architecture & design notes
├── README.md
└── LICENSE                  # Apache 2.0 license

```

## 🚀 Getting Started

1. **Add the package**  
   In TheIDE, add `upp-stateMachine` as a library dependency.

2. **Include the header**  
```cpp
   #include <StateMachine/StateMachine.h>
   using namespace Upp;
```

3. **Define states & transitions**

```cpp
   StateMachine sm;
   sm.SetInitial("Idle");

   sm.AddState({
       "Idle",
       [](auto&, auto done){ LOG("Now idle."); done(true); },
       [](auto&, auto done){ LOG("Leaving idle."); done(true); }
   });

   sm.AddState({
       "Working",
       [](auto&, auto done){
           Animate(panel).Opacity(1).Time(250).Then(done);
       },
       [](auto&, auto done){
           Animate(panel).Opacity(0).Time(250).Then(done);
       }
   });

   sm.AddTransition({
       .event    = "start",
       .from     = "Idle",
       .to       = "Working",
       .Guard    = [](auto&){ return true; },
       .OnBefore = [](auto&){ LOG("About to transition"); },
       .OnAfter  = [](auto&){ LOG("Entered Working"); }
   });

   sm.Start();
   sm.TriggerEvent("start");
```

## ⚙️ Example: GUI Test Harness

Included `examples/main.cpp`, a fully-fledged test suite using a custom `LogView`.
Open `examples/statemachine.upp` in TheIDE, build & run to see color-coded PASS/FAIL logs and a Cancel button.

## ⏪ GoBack()

Calls to `sm.GoBack()` undo the last transition—triggering exit/enter logic asynchronously and safely ignoring calls mid-transition.

## 📋 Planned Features

* **StateViewManager** for automatic view-switching
* Code-generation utilities for boilerplate FSM setup

## 📄 License

This project is licensed under **Apache License 2.0**. See [LICENSE](LICENSE) for details.


