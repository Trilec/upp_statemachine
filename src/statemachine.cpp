#include "statemachine.h"

namespace Upp {

TransitionContext::TransitionContext(StateMachine& m, String f, String t, String e)
    : machine(m), fromState(pick(f)), toState(pick(t)), event(pick(e)) {}

void StateMachine::AddState(State s) {
    states.Add(MakeOne<State>(pick(s)));
}

void StateMachine::AddTransition(Transition t) {
    transitions.Add(MakeOne<Transition>(pick(t)));
}

void StateMachine::Start() {
    ASSERT(!initial.IsEmpty());
    const State* init = FindState(initial);
    ASSERT(init);
    current = initial;
    transitionHistory.Add(MakeOne<TransitionRecord>("", initial, "__start"));
    if (init->OnEnter)
        init->OnEnter(*this, [](bool){});
}

void StateMachine::TriggerEvent(const String& e) {
    if (transitioning) return;
    const Transition* t = FindTransition(current, e);
    if (!t) return;
    
    TransitionContext ctx(*this, t->from, t->to, t->event);
    if (t->Guard && !t->Guard(ctx)) {
        return;
    }
    
    DoTransition(*t);
}

bool StateMachine::TryTransition(const Transition& t) {
    if (transitioning) return false;
    
    TransitionContext ctx(*this, t.from, t.to, t.event);
    if (t.Guard && !t.Guard(ctx)) {
        return false;
    }
    DoTransition(t);
    return true;
}

void StateMachine::GoBack() {
    if (!CanGoBack() || IsTransitioning()) {
        return;
    }

    transitionHistory.Pop();
    const One<TransitionRecord>& last_step = transitionHistory.Top();
    
    Transition back_transition;
    back_transition.from = current;
    back_transition.to = last_step->to; // Dereference the pointer
    back_transition.event = "__back";

    DoTransition(back_transition, false);
}

// --- FIX: Loop over smart pointers and dereference them ---
const State* StateMachine::FindState(const String& id) const {
    for (const auto& s_ptr : states)
        if (s_ptr->id == id) return s_ptr.Get();
    return nullptr;
}

const Transition* StateMachine::FindTransition(const String& from, const String& ev) const {
    for (const auto& t_ptr : transitions)
        if (t_ptr->from == from && t_ptr->event == ev)
            return t_ptr.Get();
    return nullptr;
}

void StateMachine::DoTransition(const Transition& t, bool record) {
    const State* fromState = FindState(t.from);
    const State* toState   = FindState(t.to);
    if (!fromState || !toState) {
        LOG("Error: Transition specifies a non-existent state.");
        return;
    }

    transitioning = true;
    TransitionContext ctx(*this, t.from, t.to, t.event);

    if (WhenTransitionStarted)
        WhenTransitionStarted(ctx);
    if (t.OnBefore)
        t.OnBefore(ctx);

    // This lambda is called AFTER the new state's OnEnter completes.
    auto on_enter_done = [this, ctx, record](bool success) {
        if (success) {
            // Transition is fully successful. Now call final hooks.
            if (WhenTransitionFinished)
                WhenTransitionFinished(ctx);
            
            // Find the original transition to fire its OnAfter hook.
            if (const Transition* t_ptr = FindTransition(ctx.fromState, ctx.event)) {
                if (t_ptr->OnAfter) {
                    t_ptr->OnAfter(ctx);
                }
            }

            Finalize(ctx, record);
        }
        // In all cases (success or failure), the transition process is over.
        transitioning = false;
    };

    // This lambda is called AFTER the old state's OnExit completes.
    auto on_exit_done = [this, toState, ctx, on_enter_done](bool success) {
        if (success) {
            // Exit was successful, update state and proceed to OnEnter.
            current = ctx.toState;
            
            if (toState && toState->OnEnter)
                toState->OnEnter(*this, on_enter_done);
            else
                on_enter_done(true); // No OnEnter, proceed directly.
        } else {
            // Exit failed, abort transition. Revert current state if needed.
            // (For now, we just stop, but a more complex FSM might revert 'current')
            LOG("Error: OnExit failed, transition aborted.");
            transitioning = false;
        }
    };

    if (fromState->OnExit)
        fromState->OnExit(*this, on_exit_done);
    else
        on_exit_done(true); // No OnExit, proceed directly.
}

void StateMachine::Finalize(const TransitionContext& ctx, bool record) {
    if (record) {
        if (transitionHistory.IsEmpty() || transitionHistory.Top()->to != ctx.toState) {
            transitionHistory.Add(MakeOne<TransitionRecord>(ctx.fromState, ctx.toState, ctx.event));
        }
    }
}

} // namespace Upp

/*
StateViewManager::StateViewManager(StateMachine* fsm, Ctrl* container)
    : sm(fsm), container(container) {
    if (sm) {
        sm->WhenTransitionFinished << [=](const TransitionContext& ctx) {
            OnStateChanged(ctx);
        };
    }
}

StateViewManager::~StateViewManager() {
    if (sm) sm->WhenTransitionFinished.Unbind(this);
}

void StateViewManager::RegisterView(const String& stateId, Ctrl* view) {
    views.Add(stateId, view);
    container->Add(view);
    view->Hide();
}

void StateViewManager::OnStateChanged(const TransitionContext& ctx) {
    String newState = ctx.toState;
    int idx = views.Find(newState);
    if (idx < 0) return;

    Ctrl* newView = views[idx];
    Ctrl* oldView = views.Find(current_view) >= 0 ? views.Get(current_view) : nullptr;

    if (newView == oldView) return;

    if (in_animation) {
        animation_token++;
        if (oldView) {
            oldView->Hide();
            oldView->SetOpacity(1);
        }
    }

    current_view = newState;
    int current_token = ++animation_token;

    if (animate && oldView && !in_animation) {
        in_animation = true;
        newView->SetRect(container->GetSize());
        newView->Show();
        newView->SetOpacity(0);

        oldView->Animate(fade_time, 
            [=] { if (current_token == animation_token) oldView->SetOpacity(0); }, 
            0,
            [=] {
                if (current_token != animation_token) return;
                oldView->Hide();
                newView->Animate(fade_time,
                    [=] { if (current_token == animation_token) newView->SetOpacity(1); },
                    0,
                    [=] { 
                        if (current_token == animation_token) in_animation = false; 
                    }
                );
            }
        );
    } else {
        if (oldView) oldView->Hide();
        newView->SetRect(container->GetSize());
        newView->Show();
        newView->SetOpacity(1);
    }
}
*/
