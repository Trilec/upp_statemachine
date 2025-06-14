// StateMachine.h – Asynchronous, event‑driven FSM for U++
// -------------------------------------------------------
// Lightweight finite‑state‑machine helper for orchestrating UI / logic flow in
// Ultimate++ applications.  100 % header‑only, no external dependencies.
//
//  • Event‑based transitions via `TriggerEvent()`
//  • Async enter / exit using `done(bool success)` callbacks
//  • Transition hooks: Guard / OnBefore / OnAfter
//  • History stack + `GoBack()`
//
// Example
// -------
//     StateMachine sm;
//     sm.SetInitial("Idle");
//     sm.AddState({"Idle",
//                  [](auto&, auto done){ LOG("Idle"); done(true); },
//                  {}});
//     sm.AddState({"Work",
//                  [](auto&, auto done){ PostCallback([=]{ done(true); }); },
//                  {}});
//     sm.AddTransition({"start", "Idle", "Work"});
//     sm.Start();
//     sm.TriggerEvent("start");
//
// -------------------------------------------------------

#pragma once

#include <Core/Core.h>

namespace Upp {

class StateMachine;

struct TransitionContext {
    StateMachine& machine;
    String        fromState;
    String        toState;
    String        event;

    TransitionContext(StateMachine& m, String f, String t, String e);
};

struct State {
    String id;
    Function<void(StateMachine&, Function<void(bool)> done)> OnEnter;
    Function<void(StateMachine&, Function<void(bool)> done)> OnExit;
};

struct Transition {
    String event;
    String from;
    String to;
    Function<void(const TransitionContext&)> OnBefore;
    Function<void(const TransitionContext&)> OnAfter;
    Function<bool(const TransitionContext&)> Guard;
};

struct TransitionRecord {
    String from;
    String to;
    String event;
    
    TransitionRecord(const String& f, const String& t, const String& e) : from(f), to(t), event(e) {}
};

class StateMachine {
public:
    // The public API remains the same, taking objects by value.
    void AddState(State s);
    void AddTransition(Transition t);
    
    void   SetInitial(const String& id)      { initial = id; }
    void   Start();
    void   TriggerEvent(const String& e);
    bool   TryTransition(const Transition& t);

    String GetCurrent() const                { return current; }
    bool   IsTransitioning() const           { return transitioning; }
    bool   CanGoBack() const                 { return transitionHistory.GetCount() > 1; }
    void   GoBack();

    Callback1<const TransitionContext&> WhenTransitionStarted;
    Callback1<const TransitionContext&> WhenTransitionFinished;

private:
    const State* FindState(const String& id) const;
    const Transition* FindTransition(const String& from, const String& ev) const;
    
    void DoTransition(const Transition& t, bool record = true);
    void Finalize(const TransitionContext& ctx, bool record);

    // --- Store smart pointers in the vectors ---
    Vector<One<State>>            states;
    Vector<One<Transition>>       transitions;
    Vector<One<TransitionRecord>> transitionHistory;

    String current;
    String initial;
    bool   transitioning = false;
};

} // namespace Upp


/*
class StateViewManager {
public:
    StateViewManager(StateMachine* fsm, Ctrl* container);
    ~StateViewManager();

    void RegisterView(const String& stateId, Ctrl* view);
    void EnableAnimation(bool b) { animate = b; }
    void SetFadeTime(int ms) { fade_time = ms; }
in
private:
    void OnStateChanged(const TransitionContext& ctx);

    StateMachine* sm = nullptr;
    Ctrl* container = nullptr;
    Index<String> views_order;
    VectorMap<String, Ctrl*> views;
    String current_view;
    bool animate = true;
    int fade_time = 250;
    bool in_animation = false;
    int animation_token = 0;
};
*/

