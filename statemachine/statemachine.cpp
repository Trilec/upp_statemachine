/*
  StateMachine: Asynchronous, general-purpose FSM for U++

  Overview:
    • Define states and transitions with optional guards and callbacks
    • Asynchronous OnEnter / OnExit handlers
    • Built-in history tracking and GoBack()

  Usage:
    1) Define machine and initial state
       StateMachine fsm;
       fsm.initial = "Idle";

    2) Add states
       // "Idle": no entry/exit behavior
       fsm.AddState({ "Idle",    nullptr,     nullptr });
       // "Working": custom entry/exit
       fsm.AddState({ "Working", enterWorking, exitWorking });

    3) Add transitions
       // Idle --"start"--> Working
       fsm.AddTransition({
         "Idle",      // from
         "Working",   // to
         "start",     // event
         nullptr,     // guard
         nullptr,     // onBefore
         nullptr      // onAfter
       });

    4) Start and fire events
       fsm.Start();
       fsm.TriggerEvent("start");

  UI Example: switch panels on button click
    // assume TopWindow has two panels: panelA, panelB and a Button btn
    StateMachine uiFsm;
    uiFsm.initial = "A";
    uiFsm.AddState({ "A",
      [&](StateMachine&, Function<void(bool)> done){
        panelA.Show(); panelB.Hide();
        done(true);
      },
      nullptr
    });
    uiFsm.AddState({ "B",
      [&](StateMachine&, Function<void(bool)> done){
        panelB.Show(); panelA.Hide();
        done(true);
      },
      nullptr
    });
    uiFsm.AddTransition({ "A","B","toB", nullptr,nullptr,nullptr });
    uiFsm.AddTransition({ "B","A","toA", nullptr,nullptr,nullptr });
    uiFsm.Start();
    btn <<= [&]{ uiFsm.TriggerEvent("toB"); };
*/

 

#include "statemachine.h"

namespace Upp {

//------------------------------------------------------------------------------
// TransitionContext carries context during a transition
//------------------------------------------------------------------------------
TransitionContext::TransitionContext(StateMachine& m, String f, String t, String e)
    : machine(m)
    , fromState(pick(f))
    , toState(pick(t))
    , event(pick(e))
{}

//------------------------------------------------------------------------------
// Add a new state definition
//------------------------------------------------------------------------------
void StateMachine::AddState(State s) {
    states.Add(MakeOne<State>(pick(s)));
}

//------------------------------------------------------------------------------
// Add a new transition definition
//------------------------------------------------------------------------------
void StateMachine::AddTransition(Transition t) {
    transitions.Add(MakeOne<Transition>(pick(t)));
}

//------------------------------------------------------------------------------
// Begin the state machine in its initial state
//------------------------------------------------------------------------------
void StateMachine::Start() {
    ASSERT(!initial.IsEmpty());
    const State* init = FindState(initial);
    ASSERT(init);

    current = initial;
    transitionHistory.Add(MakeOne<TransitionRecord>("", initial, "__start"));

    if (init->OnEnter)
        init->OnEnter(*this, [](bool){});
}

//------------------------------------------------------------------------------
// Trigger an event by name
//------------------------------------------------------------------------------
void StateMachine::TriggerEvent(const String& e) {
    if (transitioning) return;

    const Transition* t = FindTransition(current, e);
    if (!t) return;

    TransitionContext ctx(*this, t->from, t->to, t->event);
    if (t->Guard && !t->Guard(ctx))
        return;

    DoTransition(*t);
}

//------------------------------------------------------------------------------
// Attempt a transition by descriptor
//------------------------------------------------------------------------------
bool StateMachine::TryTransition(const Transition& t) {
    if (transitioning) return false;

    TransitionContext ctx(*this, t.from, t.to, t.event);
    if (t.Guard && !t.Guard(ctx))
        return false;

    DoTransition(t);
    return true;
}

//------------------------------------------------------------------------------
// Revert to previous state if possible
//------------------------------------------------------------------------------
void StateMachine::GoBack() {
    if (!CanGoBack() || IsTransitioning())
        return;

    const One<TransitionRecord>& last_step = transitionHistory.Top();

    Transition back_transition;
    back_transition.from  = current;
    back_transition.to    = last_step->from;
    back_transition.event = "__back";

    DoTransition(back_transition, false, [this](bool success) {
        if (success) {
            transitionHistory.Pop();
            DumpHistory();
        }
    });
}

//------------------------------------------------------------------------------
// Lookup helpers
//------------------------------------------------------------------------------
const State* StateMachine::FindState(const String& id) const {
    for (const auto& s : states)
        if (s->id == id)
            return s.Get();
    return nullptr;
}

const Transition* StateMachine::FindTransition(const String& from, const String& ev) const {
    for (const auto& t : transitions)
        if (t->from == from && t->event == ev)
            return t.Get();
    return nullptr;
}

//------------------------------------------------------------------------------
// Core transition logic (handles OnExit→OnEnter→OnAfter and history)
//------------------------------------------------------------------------------
void StateMachine::DoTransition(const Transition& t,
                                bool record,
                                Function<void(bool)> on_done)
{
    LOG(Format("DoTransition: %s -> %s, record=%d", t.from, t.to, int(record)));

    const State* fromState = FindState(t.from);
    const State* toState   = FindState(t.to);
    if (!fromState || !toState) {
        LOG("Error: Transition specifies a non-existent state.");
        if (on_done) on_done(false);
        return;
    }

    transitioning = true;
    TransitionContext ctx(*this, t.from, t.to, t.event);

    // OnBefore callback
    if (WhenTransitionStarted)
        WhenTransitionStarted(ctx);
    if (t.OnBefore)
        t.OnBefore(ctx);

    // Chain exit → enter → after → finalize
    auto on_enter_done = [this, ctx, record, on_done](bool success) {
        if (success) {
            if (WhenTransitionFinished)
                WhenTransitionFinished(ctx);

            if (const Transition* tp = FindTransition(ctx.fromState, ctx.event))
                if (tp->OnAfter)
                    tp->OnAfter(ctx);

            Finalize(ctx, record);
        }
        transitioning = false;
        if (on_done) on_done(success);
    };

    auto on_exit_done = [this, toState, ctx, on_enter_done](bool success) {
        if (success) {
            if (toState && toState->OnEnter) {
                toState->OnEnter(*this, [this, ctx, on_enter_done](bool enter_success) {
                    if (enter_success)
                        current = ctx.toState;
                    LOG("Transition " + String(enter_success ? "succeeded" : "failed") +
                        ": now in state " + current);
                    on_enter_done(enter_success);
                });
            }
            else {
                current = ctx.toState;
                LOG("Transition succeeded: now in state " + current);
                on_enter_done(true);
            }
        }
        else {
            LOG("Error: OnExit failed, transition aborted.");
            transitioning = false;
            on_enter_done(false);
        }
    };

    // Start exit phase
    if (fromState->OnExit)
        fromState->OnExit(*this, on_exit_done);
    else
        on_exit_done(true);
}

//------------------------------------------------------------------------------
// Record history and dump if needed
//------------------------------------------------------------------------------
void StateMachine::Finalize(const TransitionContext& ctx, bool record) {
    LOG(Format("Finalize: %s -> %s, record=%d", ctx.fromState, ctx.toState, int(record)));

    if (record) {
        // prune any divergent history
        while (!transitionHistory.IsEmpty() &&
               transitionHistory.Top()->to != ctx.fromState)
        {
            transitionHistory.Pop();
        }
        transitionHistory.Add(
            MakeOne<TransitionRecord>(ctx.fromState, ctx.toState, ctx.event));
        DumpHistory();
    }
}

} // namespace Upp
