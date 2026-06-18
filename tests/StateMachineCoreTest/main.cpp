#include <Core/Core.h>
#include <statemachine/statemachine.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
    StdLogSetup(LOG_COUT);

    {
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
        sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
        sm.AddTransition({"go", "A", "B"});
        sm.Start();
        sm.TriggerEvent("go");
        ASSERT(sm.GetCurrent() == "B");
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
        sm.Start();
        sm.TriggerEvent("start");
        ASSERT(sm.GetCurrent() == "Idle");
        allow = true;
        sm.TriggerEvent("start");
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
        sm.Start();
        sm.TriggerEvent("go");
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
        sm.Start();
        sm.TriggerEvent("go");
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
        sm.Start();
        sm.TriggerEvent("to_b");
        sm.TriggerEvent("to_c");
        sm.GoBack();
        ASSERT(sm.GetCurrent() == "B");
    }
}
