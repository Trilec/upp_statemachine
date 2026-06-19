#include <Core/Core.h>
#include <statemachine/statemachine.h>

using namespace Upp;

struct TestContext {
    bool passed = true;

    void Check(bool condition, const String& message) {
        if(!condition) {
            passed = false;
            Cout() << "    CHECK FAILED: " << message << "\n";
        }
    }
};

bool HasString(const Vector<String>& values, const String& needle) {
    for (int i = 0; i < values.GetCount(); ++i) {
        if (values[i] == needle)
            return true;
    }
    return false;
}

bool SameOrder(const Vector<String>& actual, const Vector<String>& expected) {
    if (actual.GetCount() != expected.GetCount())
        return false;
    for (int i = 0; i < expected.GetCount(); ++i) {
        if (actual[i] != expected[i])
            return false;
    }
    return true;
}

template <class Fn>
bool RunTest(const String& name, Fn fn) {
    TestContext ctx;
    fn(ctx);
    Cout() << name << ": " << (ctx.passed ? "PASSED" : "FAILED") << "\n";
    return ctx.passed;
}

template <class Fn>
void RunGroup(const String& name, int& passed, int& failed, Fn fn) {
    Cout() << "\n== " << name << " ==\n";
    fn([&](const String& test_name, auto test_fn) {
        if (RunTest(test_name, test_fn))
            ++passed;
        else
            ++failed;
    });
}

CONSOLE_APP_MAIN
{
    StdLogSetup(LOG_COUT);

    int passed = 0;
    int failed = 0;

    RunGroup("Startup", passed, failed, [&](auto add) {
        add("Start valid initial state", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be started");
            ctx.Check(!sm.IsTransitioning(), "Machine should not be transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Current state should be A");
        });

        add("Start missing initial state rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("Missing");

            ctx.Check(!sm.Start(), "Start() should return false");
            ctx.Check(!sm.IsStarted(), "Machine should not be started");
        });

        add("Start empty initial state rejected", [](TestContext& ctx) {
            StateMachine sm;

            ctx.Check(!sm.Start(), "Start() should return false");
            ctx.Check(!sm.IsStarted(), "Machine should not be started");
        });

        add("Start cannot run twice", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "First Start() should return true");
            ctx.Check(!sm.Start(), "Second Start() should return false");
        });

        add("Start without OnEnter completes immediately", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", {}, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be started");
            ctx.Check(!sm.IsTransitioning(), "Machine should not be transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Current state should be A");
        });

        add("Start async OnEnter success", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                ctx.Check(sm.IsTransitioning(), "Start() should set transitioning before initial OnEnter");
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be marked started while initial async OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while initial OnEnter is pending");

            finish_start(true);

            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after initial OnEnter completes");
            ctx.Check(sm.IsStarted(), "Machine should remain started after successful initial OnEnter");
            ctx.Check(sm.GetCurrent() == "A", "Current state should be A");
        });

        add("Start async OnEnter failure rollback", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                ctx.Check(sm.IsTransitioning(), "Start() should set transitioning before initial OnEnter");
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be marked started while initial async OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while initial OnEnter is pending");

            finish_start(false);

            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after initial OnEnter fails");
            ctx.Check(!sm.IsStarted(), "Machine should not remain started after initial OnEnter failure");
            ctx.Check(sm.GetCurrent().IsEmpty(), "Current state should be cleared after initial OnEnter failure");
            ctx.Check(!sm.CanGoBack(), "History should be cleared after initial OnEnter failure");
        });

        add("TriggerEvent before Start rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should return false");
        });

        add("TryTransition before Start rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";

            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
        });

        add("GoBack before Start ignored", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            sm.GoBack();
            ctx.Check(!sm.IsStarted(), "Machine should not be started");
            ctx.Check(!sm.IsTransitioning(), "Machine should not be transitioning");
            ctx.Check(sm.GetCurrent().IsEmpty(), "Current state should stay empty");
        });
    });

    RunGroup("TriggerEvent", passed, failed, [&](auto add) {
        add("Basic A to B transition", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Basic A->B transition should return true");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B");
        });

        add("Unknown event rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.TriggerEvent("unknown"), "Unknown event should return false");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A");
        });

        add("Event for different source state rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go_b", "B", "C"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.TriggerEvent("go_b"), "Event for different source state should return false");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A");
        });

        add("Guard blocked transition", [](TestContext& ctx) {
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

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.TriggerEvent("start"), "Guard-blocked event should return false");
            ctx.Check(sm.GetCurrent() == "Idle", "Current state should stay Idle");
        });

        add("Valid event accepted", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Valid event should return true");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B");
        });

        add("TriggerEvent while transitioning rejected", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning during initial OnEnter");
            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should return false while transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A while transitioning");

            finish_start(true);
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after initial OnEnter completes");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after startup");
        });

        add("Transition to missing target fails safely", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "Missing"});

            ctx.Check(sm.Start(), "Start() should return true");

            bool accepted = sm.TriggerEvent("go");
            ctx.Check(accepted, "Current behavior accepts the event before DoTransition fails");
            if (accepted)
                Cout() << "    NOTE: TriggerEvent() accepted a missing target transition; DoTransition() failed later. Design issue for next pass.\n";

            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A after missing target failure");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning after missing target failure");
        });
    });

    RunGroup("TryTransition", passed, failed, [&](auto add) {
        add("TryTransition before Start rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";

            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetCurrent().IsEmpty(), "Current state should remain empty before Start()");
        });

        add("TryTransition while transitioning rejected", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning during initial OnEnter");

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";

            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false while transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A while transitioning");

            finish_start(true);
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after startup");
        });

        add("TryTransition from wrong current state rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "B";
            t.to = "A";

            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A");
        });

        add("TryTransition to missing state rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "Missing";

            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A");
        });

        add("Valid TryTransition accepted", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            int after_count = 0;
            t.OnAfter = [&](const TransitionContext&) {
                ++after_count;
            };

            ctx.Check(sm.TryTransition(t), "TryTransition() should return true");
            ctx.Check(after_count == 1, "OnAfter should fire exactly once");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B");
        });
    });

    RunGroup("Failure rollback", passed, failed, [&](auto add) {
        add("Failed OnExit keeps current state", [](TestContext& ctx) {
            StateMachine sm;
            int enter_a = 0;
            int exit_a = 0;
            int enter_b = 0;
            sm.SetInitial("A");
            sm.AddState({"A",
                [&](auto&, auto done) {
                    ++enter_a;
                    done(true);
                },
                [&](auto&, auto done) {
                    ++exit_a;
                    done(false);
                }
            });
            sm.AddState({"B",
                [&](auto&, auto done) {
                    ++enter_b;
                    done(true);
                },
                {}
            });
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnExit");
            ctx.Check(enter_a == 1, "A OnEnter should run once");
            ctx.Check(exit_a == 1, "A OnExit should run once");
            ctx.Check(enter_b == 0, "B OnEnter should not be called when OnExit fails");
        });

        add("Failed OnExit does not call target OnEnter", [](TestContext& ctx) {
            StateMachine sm;
            int enter_b = 0;
            int exit_a = 0;
            sm.SetInitial("A");
            sm.AddState({"A",
                [](auto&, auto done) { done(true); },
                [&](auto&, auto done) {
                    ++exit_a;
                    done(false);
                }
            });
            sm.AddState({"B",
                [&](auto&, auto done) {
                    ++enter_b;
                    done(true);
                },
                {}
            });
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnExit");
            ctx.Check(exit_a == 1, "A OnExit should run once");
            ctx.Check(enter_b == 0, "B OnEnter should not be called");
        });

        add("Failed OnEnter keeps old current state", [](TestContext& ctx) {
            StateMachine sm;
            int enter_b = 0;
            int exit_a = 0;
            sm.SetInitial("A");
            sm.AddState({"A",
                [](auto&, auto done) { done(true); },
                [&](auto&, auto done) {
                    ++exit_a;
                    done(true);
                }
            });
            sm.AddState({"B",
                [&](auto&, auto done) {
                    ++enter_b;
                    done(false);
                },
                [](auto&, auto done) { done(true); }
            });
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnEnter");
            ctx.Check(exit_a == 1, "A OnExit should run once");
            ctx.Check(enter_b == 1, "B OnEnter should be called once");
        });

        add("Failed OnEnter does not record history", [](TestContext& ctx) {
            StateMachine sm;
            int enter_b = 0;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B",
                [&](auto&, auto done) {
                    ++enter_b;
                    done(false);
                },
                {}
            });
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnEnter");
            ctx.Check(!sm.CanGoBack(), "Failed OnEnter should not add history");
            ctx.Check(enter_b == 1, "B OnEnter should have been attempted once");
        });

        add("Failed OnEnter clears transitioning flag", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(false); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(!sm.IsTransitioning(), "Machine should clear transitioning after failed OnEnter");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A");
        });

        add("Failed OnEnter allows later valid event from old state", [](TestContext& ctx) {
            StateMachine sm;
            bool allow = false;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B",
                [&](auto&, auto done) {
                    if (allow)
                        done(true);
                    else
                        done(false);
                },
                {}
            });
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "First transition should begin");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after failure");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnEnter");

            allow = true;
            ctx.Check(sm.TriggerEvent("go"), "Later valid event from old state should return true");
            ctx.Check(sm.GetCurrent() == "B", "Current state should become B after later valid event");
        });
    });

    RunGroup("GoBack/history", passed, failed, [&](auto add) {
        add("CanGoBack false after Start", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after Start()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "GoBack() at the start should do nothing");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should still be false at the start");
        });

        add("CanGoBack true after one transition", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after Start()");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B after A->B");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true after one transition");
        });

        add("GoBack from B returns to A", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B before GoBack()");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "GoBack() should return to A");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after returning to A");
        });

        add("GoBack from C returns to B", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});
            sm.AddTransition({"to_c", "B", "C"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.TriggerEvent("to_c"), "B->C should begin");
            ctx.Check(sm.GetCurrent() == "C", "Current state should be C before GoBack()");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "B", "GoBack() should return to B");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should remain true after one GoBack()");
        });

        add("GoBack twice from C returns to A", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});
            sm.AddTransition({"to_c", "B", "C"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.TriggerEvent("to_c"), "B->C should begin");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before first GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "B", "First GoBack() should return to B");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should still be true after first GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "Second GoBack() should return to A");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after returning to A");
        });

        add("GoBack at start does nothing", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false at the start");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "GoBack() at the start should not change state");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should still be false at the start");
        });

        add("GoBack does not require registered reverse transition", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B before GoBack()");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "GoBack() should return to A without a reverse transition");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after returning to A");
        });

        add("GoBack failed OnExit keeps current state", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"C",
                [](auto&, auto done) { done(true); },
                [](auto&, auto done) { done(false); }
            });
            sm.AddTransition({"to_b", "A", "B"});
            sm.AddTransition({"to_c", "B", "C"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.TriggerEvent("to_c"), "B->C should begin");
            ctx.Check(sm.GetCurrent() == "C", "Current state should be C before failed GoBack()");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before failed GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "C", "Failed GoBack() OnExit should keep current state C");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should remain true after failed GoBack()");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning after failed GoBack()");
        });

        add("GoBack failed OnEnter keeps current state", [](TestContext& ctx) {
            StateMachine sm;
            int enter_b = 0;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B",
                [&](auto&, auto done) {
                    ++enter_b;
                    done(enter_b == 1);
                },
                [](auto&, auto done) { done(true); }
            });
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});
            sm.AddTransition({"to_c", "B", "C"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.TriggerEvent("to_c"), "B->C should begin");
            ctx.Check(sm.GetCurrent() == "C", "Current state should be C before failed GoBack()");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before failed GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "C", "Failed GoBack() OnEnter should keep current state C");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should remain true after failed GoBack()");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning after failed GoBack()");
            ctx.Check(enter_b == 2, "B OnEnter should be attempted twice");
        });

        add("New branch after GoBack prunes divergent history", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"D", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});
            sm.AddTransition({"to_c", "B", "C"});
            sm.AddTransition({"to_d", "B", "D"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.TriggerEvent("to_c"), "B->C should begin");
            ctx.Check(sm.GetCurrent() == "C", "Current state should be C before branching");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before first GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "B", "GoBack() should return to B before branching");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should still be true after returning to B");
            ctx.Check(sm.TriggerEvent("to_d"), "B->D should begin on the new branch");
            ctx.Check(sm.GetCurrent() == "D", "Current state should be D after branching");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true after branching");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "B", "GoBack() from the branch should return to B");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should still be true after returning to B");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "Second GoBack() from the branch should return to A");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after pruning divergent history");
        });
    });

    RunGroup("Callback ordering", passed, failed, [&](auto add) {
        add("Successful transition callback order", [](TestContext& ctx) {
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

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(sm.TriggerEvent("start"), "Transition should begin");

            Vector<String> expected;
            expected.Add("WhenTransitionStarted");
            expected.Add("OnBefore");
            expected.Add("OnExit");
            expected.Add("OnEnter");
            expected.Add("WhenTransitionFinished");
            expected.Add("OnAfter");

            ctx.Check(order.GetCount() == expected.GetCount(), "Callback count should match");
            for (int i = 0; i < expected.GetCount(); ++i)
                ctx.Check(order[i] == expected[i], "Callback order mismatch at step " + AsString(i));
        });

        add("Guard false calls only Guard", [](TestContext& ctx) {
            StateMachine sm;
            Vector<String> order;
            bool allow = false;

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
            t.Guard = [&](const TransitionContext&) {
                order.Add("Guard");
                return allow;
            };
            t.OnBefore = [&](const TransitionContext&) { order.Add("OnBefore"); };
            t.OnAfter = [&](const TransitionContext&) { order.Add("OnAfter"); };
            sm.WhenTransitionStarted = [&](const TransitionContext&) {
                order.Add("WhenTransitionStarted");
            };
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                order.Add("WhenTransitionFinished");
            };

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(!sm.TriggerEvent("start"), "Guard-blocked transition should return false");

            Vector<String> expected;
            expected.Add("Guard");
            ctx.Check(SameOrder(order, expected), "Guard false should call only Guard");
            ctx.Check(sm.GetCurrent() == "Idle", "Current state should stay Idle");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning");
        });

        add("OnExit failure calls Started, OnBefore, OnExit, then stops", [](TestContext& ctx) {
            StateMachine sm;
            Vector<String> order;

            sm.SetInitial("Idle");
            sm.AddState({
                "Idle",
                [&](auto&, auto done) {
                    order.Add("OnEnter");
                    done(true);
                },
                [&](auto&, auto done) {
                    order.Add("OnExit");
                    done(false);
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
            t.Guard = [&](const TransitionContext&) {
                order.Add("Guard");
                return true;
            };
            t.OnBefore = [&](const TransitionContext&) { order.Add("OnBefore"); };
            t.OnAfter = [&](const TransitionContext&) { order.Add("OnAfter"); };
            sm.WhenTransitionStarted = [&](const TransitionContext&) {
                order.Add("WhenTransitionStarted");
            };
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                order.Add("WhenTransitionFinished");
            };

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(!sm.TriggerEvent("start"), "Failed OnExit should return false");

            Vector<String> expected;
            expected.Add("WhenTransitionStarted");
            expected.Add("OnBefore");
            expected.Add("OnExit");

            ctx.Check(SameOrder(order, expected), "Failed OnExit should stop after OnExit");
            ctx.Check(sm.GetCurrent() == "Idle", "Current state should stay Idle");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnExit");
        });

        add("OnEnter failure calls Started, OnBefore, OnExit, OnEnter, then stops", [](TestContext& ctx) {
            StateMachine sm;
            Vector<String> order;

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
                    done(false);
                },
                {}
            });

            Transition t;
            t.event = "start";
            t.from = "Idle";
            t.to = "Working";
            t.Guard = [&](const TransitionContext&) {
                order.Add("Guard");
                return true;
            };
            t.OnBefore = [&](const TransitionContext&) { order.Add("OnBefore"); };
            t.OnAfter = [&](const TransitionContext&) { order.Add("OnAfter"); };
            sm.WhenTransitionStarted = [&](const TransitionContext&) {
                order.Add("WhenTransitionStarted");
            };
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                order.Add("WhenTransitionFinished");
            };

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(!sm.TriggerEvent("start"), "Failed OnEnter should return false");

            Vector<String> expected;
            expected.Add("WhenTransitionStarted");
            expected.Add("OnBefore");
            expected.Add("OnExit");
            expected.Add("OnEnter");

            ctx.Check(SameOrder(order, expected), "Failed OnEnter should stop after OnEnter");
            ctx.Check(sm.GetCurrent() == "Idle", "Current state should stay Idle");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnEnter");
        });

        add("OnAfter is not called on failed OnExit", [](TestContext& ctx) {
            StateMachine sm;
            Vector<String> order;

            sm.SetInitial("Idle");
            sm.AddState({
                "Idle",
                [&](auto&, auto done) {
                    order.Add("OnEnter");
                    done(true);
                },
                [&](auto&, auto done) {
                    order.Add("OnExit");
                    done(false);
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
            t.Guard = [&](const TransitionContext&) {
                order.Add("Guard");
                return true;
            };
            t.OnBefore = [&](const TransitionContext&) { order.Add("OnBefore"); };
            t.OnAfter = [&](const TransitionContext&) { order.Add("OnAfter"); };
            sm.WhenTransitionStarted = [&](const TransitionContext&) {
                order.Add("WhenTransitionStarted");
            };
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                order.Add("WhenTransitionFinished");
            };

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(!sm.TriggerEvent("start"), "Failed OnExit should return false");

            ctx.Check(!HasString(order, "OnAfter"), "OnAfter should not be called when OnExit fails");
            ctx.Check(!HasString(order, "WhenTransitionFinished"), "WhenTransitionFinished should not be called when OnExit fails");
        });

        add("OnAfter is not called on failed OnEnter", [](TestContext& ctx) {
            StateMachine sm;
            Vector<String> order;

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
                    done(false);
                },
                {}
            });

            Transition t;
            t.event = "start";
            t.from = "Idle";
            t.to = "Working";
            t.Guard = [&](const TransitionContext&) {
                order.Add("Guard");
                return true;
            };
            t.OnBefore = [&](const TransitionContext&) { order.Add("OnBefore"); };
            t.OnAfter = [&](const TransitionContext&) { order.Add("OnAfter"); };
            sm.WhenTransitionStarted = [&](const TransitionContext&) {
                order.Add("WhenTransitionStarted");
            };
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                order.Add("WhenTransitionFinished");
            };

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(!sm.TriggerEvent("start"), "Failed OnEnter should return false");

            ctx.Check(!HasString(order, "OnAfter"), "OnAfter should not be called when OnEnter fails");
            ctx.Check(!HasString(order, "WhenTransitionFinished"), "WhenTransitionFinished should not be called when OnEnter fails");
        });
    });

    RunGroup("Async completion", passed, failed, [&](auto add) {
        add("Async startup completion", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                ctx.Check(sm.IsTransitioning(), "Start() should set transitioning before initial OnEnter");
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be marked started while initial async OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while initial OnEnter is pending");

            finish_start(true);

            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after initial OnEnter completes");
            ctx.Check(sm.IsStarted(), "Machine should remain started after successful initial OnEnter");
            ctx.Check(sm.GetCurrent() == "A", "Current state should be A");
        });
    });

    RunGroup("Stress", passed, failed, [&](auto add) {
        add("Stress 1,000 simple A-B-A cycles", [](TestContext& ctx) {
            StateMachine sm;
            int to_b_count = 0;
            int to_a_count = 0;
            bool ok = true;

            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});
            sm.AddTransition({"to_a", "B", "A"});

            ok &= sm.Start();
            for (int i = 0; i < 1000; ++i) {
                if (sm.TriggerEvent("to_b"))
                    ++to_b_count;
                else
                    ok = false;
                if (sm.GetCurrent() != "B")
                    ok = false;

                if (sm.TriggerEvent("to_a"))
                    ++to_a_count;
                else
                    ok = false;
                if (sm.GetCurrent() != "A")
                    ok = false;
            }

            ctx.Check(ok, "A-B-A cycles should all succeed");
            ctx.Check(to_b_count == 1000, "A->B should run 1,000 times");
            ctx.Check(to_a_count == 1000, "B->A should run 1,000 times");
            ctx.Check(sm.GetCurrent() == "A", "Current state should end on A");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning after the stress run");
        });

        add("Stress 1,000 guard checks", [](TestContext& ctx) {
            StateMachine sm;
            int guard_count = 0;
            int rejected_count = 0;
            bool ok = true;

            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.Guard = [&](const TransitionContext&) {
                ++guard_count;
                return false;
            };
            sm.AddTransition(t);

            ok &= sm.Start();
            for (int i = 0; i < 1000; ++i) {
                if (!sm.TriggerEvent("go"))
                    ++rejected_count;
                else
                    ok = false;
                if (sm.GetCurrent() != "A")
                    ok = false;
            }

            ctx.Check(ok, "Guard checks should all reject the transition");
            ctx.Check(guard_count == 1000, "Guard should run 1,000 times");
            ctx.Check(rejected_count == 1000, "TriggerEvent() should reject 1,000 times");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A");
        });

        add("Stress 1,000 TryTransition calls", [](TestContext& ctx) {
            StateMachine sm;
            int success_count = 0;
            bool ok = true;

            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});

            Transition to_b;
            to_b.event = "to_b";
            to_b.from = "A";
            to_b.to = "B";

            Transition to_a;
            to_a.event = "to_a";
            to_a.from = "B";
            to_a.to = "A";

            ok &= sm.Start();
            for (int i = 0; i < 500; ++i) {
                if (sm.TryTransition(to_b))
                    ++success_count;
                else
                    ok = false;
                if (sm.GetCurrent() != "B")
                    ok = false;

                if (sm.TryTransition(to_a))
                    ++success_count;
                else
                    ok = false;
                if (sm.GetCurrent() != "A")
                    ok = false;
            }

            ctx.Check(ok, "TryTransition() should succeed for each alternating call");
            ctx.Check(success_count == 1000, "TryTransition() should succeed 1,000 times");
            ctx.Check(sm.GetCurrent() == "A", "Current state should end on A");
        });

        add("Stress 1,000 GoBack operations across a three-state path", [](TestContext& ctx) {
            StateMachine sm;
            int go_back_count = 0;
            bool ok = true;

            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});
            sm.AddTransition({"to_c", "B", "C"});

            ok &= sm.Start();
            for (int i = 0; i < 500; ++i) {
                if (!sm.TriggerEvent("to_b"))
                    ok = false;
                if (sm.GetCurrent() != "B")
                    ok = false;
                if (!sm.TriggerEvent("to_c"))
                    ok = false;
                if (sm.GetCurrent() != "C")
                    ok = false;

                if (!sm.CanGoBack())
                    ok = false;
                sm.GoBack();
                ++go_back_count;
                if (sm.GetCurrent() != "B")
                    ok = false;
                if (!sm.CanGoBack())
                    ok = false;
                sm.GoBack();
                ++go_back_count;
                if (sm.GetCurrent() != "A")
                    ok = false;
            }

            ctx.Check(ok, "GoBack() should preserve the A-B-C path across 1,000 operations");
            ctx.Check(go_back_count == 1000, "GoBack() should run 1,000 times");
            ctx.Check(sm.GetCurrent() == "A", "Current state should end on A");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning after the stress run");
        });

        add("Stress repeated failed transitions do not corrupt current state", [](TestContext& ctx) {
            StateMachine sm;
            int enter_b = 0;
            int rejected_count = 0;
            bool ok = true;

            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B",
                [&](auto&, auto done) {
                    ++enter_b;
                    done(false);
                },
                {}
            });
            sm.AddTransition({"go", "A", "B"});

            ok &= sm.Start();
            for (int i = 0; i < 1000; ++i) {
                if (!sm.TriggerEvent("go"))
                    ++rejected_count;
                else
                    ok = false;
                if (sm.GetCurrent() != "A")
                    ok = false;
                if (sm.IsTransitioning())
                    ok = false;
            }

            ctx.Check(ok, "Failed transitions should not corrupt the current state");
            ctx.Check(enter_b == 1000, "B OnEnter should be attempted 1,000 times");
            ctx.Check(rejected_count == 1000, "TriggerEvent() should reject 1,000 times");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning");
        });
    });

    Cout() << "\n----------------------------------------\n";
    Cout() << "StateMachineCoreTest summary\n";
    Cout() << "Passed: " << passed << "\n";
    Cout() << "Failed: " << failed << "\n";
    Cout() << "Total:  " << (passed + failed) << "\n";
    Cout() << "----------------------------------------\n";
    Cout() << (failed == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
}
