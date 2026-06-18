#include <Core/Core.h>
#include <statemachine/statemachine.h>
#include <thread>
#include <chrono>

using namespace Upp;

static void WaitForCompletion(StateMachine& sm)
{
    while(sm.IsTransitioning())
        Sleep(1);
}

CONSOLE_APP_MAIN
{
    StdLogSetup(LOG_COUT);

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
        sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
        sm.AddTransition({"go", "A", "B"});
        ASSERT(sm.Start());
        ASSERT(sm.IsStarted());
        ASSERT(!sm.TriggerEvent("unknown"));
        ASSERT(sm.TriggerEvent("go"));
        ASSERT(sm.GetCurrent() == "B");
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", {}, {}});
        ASSERT(sm.Start());
        ASSERT(sm.IsStarted());
        ASSERT(!sm.IsTransitioning());
        ASSERT(sm.GetCurrent() == "A");
    }

    {
        StateMachine sm;
        bool allow = false;
        sm.SetInitial("Idle");
        sm.AddState({"Idle", [](auto&, auto done) { done(true); }, {}});
        sm.AddState({"Working", [](auto&, auto done) { done(true); }, {}});
        Transition t;
        t.event = "start";
        t.from = "Idle";
        t.to = "Working";
        t.Guard = [&](const TransitionContext&) { return allow; };
        sm.AddTransition(t);
        ASSERT(sm.Start());
        ASSERT(sm.IsStarted());
        ASSERT(!sm.TriggerEvent("start"));
        ASSERT(sm.GetCurrent() == "Idle");
        allow = true;
        ASSERT(sm.TriggerEvent("start"));
        ASSERT(sm.GetCurrent() == "Working");
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A",
            [](auto&, auto done) { done(true); },
            [](auto&, auto done) { done(false); }
        });
        sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
        sm.AddTransition({"go", "A", "B"});
        ASSERT(sm.Start());
        ASSERT(sm.TriggerEvent("go"));
        ASSERT(sm.GetCurrent() == "A");
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
        sm.AddState({"B",
            [](auto&, auto done) { done(false); },
            [](auto&, auto done) { done(true); }
        });
        sm.AddTransition({"go", "A", "B"});
        ASSERT(sm.Start());
        ASSERT(sm.TriggerEvent("go"));
        ASSERT(sm.GetCurrent() == "A");
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
        sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
        sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
        sm.AddTransition({"to_b", "A", "B"});
        sm.AddTransition({"to_c", "B", "C"});
        ASSERT(sm.Start());
        ASSERT(sm.TriggerEvent("to_b"));
        ASSERT(sm.TriggerEvent("to_c"));
        sm.GoBack();
        ASSERT(sm.GetCurrent() == "B");
    }

    {
        StateMachine sm;
        ASSERT(!sm.Start());
        ASSERT(!sm.IsStarted());
    }

    {
        StateMachine sm;
        sm.SetInitial("Missing");
        ASSERT(!sm.Start());
        ASSERT(!sm.IsStarted());
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
        ASSERT(sm.Start());
        ASSERT(!sm.Start());
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
        sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
        ASSERT(sm.Start());

        Transition wrong_from;
        wrong_from.event = "go";
        wrong_from.from = "B";
        wrong_from.to = "A";
        ASSERT(!sm.TryTransition(wrong_from));

        Transition missing_to;
        missing_to.event = "go";
        missing_to.from = "A";
        missing_to.to = "Missing";
        ASSERT(!sm.TryTransition(missing_to));

        Transition valid;
        valid.event = "go";
        valid.from = "A";
        valid.to = "B";
        int after_count = 0;
        valid.OnAfter = [&](const TransitionContext&) {
            ++after_count;
        };
        ASSERT(sm.TryTransition(valid));
        ASSERT(after_count == 1);
        ASSERT(sm.GetCurrent() == "B");
    }

    {
        StateMachine sm;
        Vector<String> order;
        bool allow = true;

        sm.SetInitial("Idle");
        sm.AddState({
            "Idle",
            [&](auto&, auto done) {
                order.Add("OnEnter");
                done(true);
            },
            [&](auto&, auto done) {
                order.Add("OnExit");
                done(true);
            }
        });
        sm.AddState({
            "Working",
            [&](auto&, auto done) {
                order.Add("OnEnter");
                done(true);
            },
            {}
        });

        Transition t;
        t.event = "start";
        t.from = "Idle";
        t.to = "Working";
        t.Guard = [&](const TransitionContext&) { return allow; };
        t.OnBefore = [&](const TransitionContext&) { order.Add("OnBefore"); };
        t.OnAfter = [&](const TransitionContext&) { order.Add("OnAfter"); };
        sm.WhenTransitionStarted = [&](const TransitionContext&) {
            order.Add("WhenTransitionStarted");
        };
        sm.WhenTransitionFinished = [&](const TransitionContext&) {
            order.Add("WhenTransitionFinished");
        };

        ASSERT(sm.Start());
        order.Clear();
        ASSERT(sm.TriggerEvent("start"));

        Vector<String> expected;
        expected.Add("WhenTransitionStarted");
        expected.Add("OnBefore");
        expected.Add("OnExit");
        expected.Add("OnEnter");
        expected.Add("WhenTransitionFinished");
        expected.Add("OnAfter");

        ASSERT(order.GetCount() == expected.GetCount());
        for (int i = 0; i < expected.GetCount(); ++i)
            ASSERT(order[i] == expected[i]);
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
        ASSERT(!sm.TriggerEvent("go"));
    }

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [&sm](auto&, auto done) {
            ASSERT(sm.IsTransitioning());
            std::thread([done]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                done(true);
            }).detach();
        }, {}});
        ASSERT(sm.Start());
        ASSERT(sm.IsStarted());
        ASSERT(sm.IsTransitioning());
        WaitForCompletion(sm);
        ASSERT(!sm.IsTransitioning());
    }
}
