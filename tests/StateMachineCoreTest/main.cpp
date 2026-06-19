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

    RunGroup("Configuration", passed, failed, [&](auto add) {
        add("AddState valid state accepted", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "AddState() should return true");
        });

        add("AddState empty id rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(!sm.AddState({"", [](auto&, auto done) { done(true); }, {}}), "AddState() should return false");
        });

        add("AddState duplicate id rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "First AddState() should return true");
            ctx.Check(!sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "Duplicate AddState() should return false");
        });

        add("AddState after Start rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "AddState() should return true before Start()");
            sm.SetInitial("A");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "AddState() should return false after Start()");
        });

        add("Logging disabled by default", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(!sm.IsLoggingEnabled(), "Logging should be disabled by default");
        });

        add("EnableLogging true enables logging flag", [](TestContext& ctx) {
            StateMachine sm;
            sm.EnableLogging(true);
            ctx.Check(sm.IsLoggingEnabled(), "Logging flag should be enabled");
        });

        add("EnableLogging false disables logging flag", [](TestContext& ctx) {
            StateMachine sm;
            sm.EnableLogging(true);
            sm.EnableLogging(false);
            ctx.Check(!sm.IsLoggingEnabled(), "Logging flag should be disabled");
        });

        add("AddTransition valid transition accepted", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "AddTransition() should return true");
        });

        add("AddTransition empty event rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(!sm.AddTransition({"", "A", "B"}), "AddTransition() should return false");
        });

        add("AddTransition empty from rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(!sm.AddTransition({"go", "", "B"}), "AddTransition() should return false");
        });

        add("AddTransition empty to rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(!sm.AddTransition({"go", "A", ""}), "AddTransition() should return false");
        });

        add("AddTransition missing from rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(!sm.AddTransition({"go", "A", "B"}), "AddTransition() should return false");
        });

        add("AddTransition missing to rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(!sm.AddTransition({"go", "A", "B"}), "AddTransition() should return false");
        });

        add("AddTransition duplicate from-event rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(sm.AddState({"C", [](auto&, auto done) { done(true); }, {}}), "State C should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "First AddTransition() should return true");
            ctx.Check(!sm.AddTransition({"go", "A", "C"}), "Duplicate AddTransition() should return false");
        });

        add("AddTransition after Start rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added before Start()");
            sm.SetInitial("A");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.AddTransition({"go2", "A", "B"}), "AddTransition() should return false after Start()");
        });
    });

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

        add("TriggerEvent missing target rejected", [](TestContext& ctx) {
            StateMachine sm;
            int guard_count = 0;
            int started_count = 0;
            int before_count = 0;
            int exit_count = 0;
            int enter_count = 0;
            int finished_count = 0;
            int after_count = 0;

            sm.SetInitial("A");
            sm.AddState({
                "A",
                [&](auto&, auto done) {
                    ++enter_count;
                    done(true);
                },
                [&](auto&, auto done) {
                    ++exit_count;
                    done(true);
                }
            });
            sm.WhenTransitionStarted = [&](const TransitionContext&) {
                ++started_count;
            };
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                ++finished_count;
            };

            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "Missing";
            t.Guard = [&](const TransitionContext&) {
                ++guard_count;
                return true;
            };
            t.OnBefore = [&](const TransitionContext&) { ++before_count; };
            t.OnAfter = [&](const TransitionContext&) { ++after_count; };
            sm.AddTransition(t);

            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should return false");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A after missing target failure");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning after missing target failure");
            ctx.Check(guard_count == 0, "Guard should not run when target state is missing");
            ctx.Check(started_count == 0, "WhenTransitionStarted should not run when target state is missing");
            ctx.Check(before_count == 0, "OnBefore should not run when target state is missing");
            ctx.Check(exit_count == 0, "OnExit should not run when target state is missing");
            ctx.Check(enter_count == 1, "Initial OnEnter should run once during Start()");
            ctx.Check(finished_count == 0, "WhenTransitionFinished should not run when target state is missing");
            ctx.Check(after_count == 0, "OnAfter should not run when target state is missing");
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
        add("History count after Start is 1", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.GetHistoryCount() == 1, "History count should be 1 after Start()");
            ctx.Check(sm.GetHistoryFrom(0) == "", "Start history from-state should be empty");
            ctx.Check(sm.GetHistoryTo(0) == "A", "Start history to-state should be A");
            ctx.Check(sm.GetHistoryEvent(0) == "__start", "Start history event should be __start");
            ctx.Check(sm.GetHistoryFrom(-1).IsEmpty(), "Invalid history from-state should be empty");
            ctx.Check(sm.GetHistoryTo(1).IsEmpty(), "Invalid history to-state should be empty");
            ctx.Check(sm.GetHistoryEvent(1).IsEmpty(), "Invalid history event should be empty");
        });

        add("History count after one transition is 2", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetHistoryCount() == 2, "History count should be 2 after one transition");
        });

        add("History records transition event", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetHistoryFrom(1) == "A", "History from-state should be A");
            ctx.Check(sm.GetHistoryTo(1) == "B", "History to-state should be B");
            ctx.Check(sm.GetHistoryEvent(1) == "go", "History event should be go");
        });

        add("Failed transition does not add history", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(false); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetHistoryCount() == 1, "Failed transition should not add history");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after failed transition");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed transition");
        });

        add("GoBack pops history", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.GetHistoryCount() == 2, "History count should be 2 before GoBack()");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before GoBack()");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "GoBack() should return to A");
            ctx.Check(sm.GetHistoryCount() == 1, "GoBack() should pop history");
            ctx.Check(sm.GetHistoryTo(0) == "A", "Remaining history entry should point to A");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after GoBack()");
        });

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
            sm.AddTransition(t);

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
            sm.AddTransition(t);

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
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(sm.TriggerEvent("start"), "Failed OnExit should begin");

            Vector<String> expected;
            expected.Add("Guard");
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
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(sm.TriggerEvent("start"), "Failed OnEnter should begin");

            Vector<String> expected;
            expected.Add("Guard");
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
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(sm.TriggerEvent("start"), "Failed OnExit should begin");

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
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            order.Clear();
            ctx.Check(sm.TriggerEvent("start"), "Failed OnEnter should begin");

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

        add("Async OnExit keeps machine transitioning until done", [](TestContext& ctx) {
            Function<void(bool)> finish_exit;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](StateMachine&, Function<void(bool)> done) {
                finish_exit = pick(done);
            }});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while OnExit is pending");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A while OnExit is pending");

            finish_exit(true);
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after OnExit completes");
            ctx.Check(sm.GetCurrent() == "B", "Current state should become B after OnExit completes");
        });

        add("Async OnEnter keeps machine transitioning until done", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                finish_enter = pick(done);
            }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while OnEnter is pending");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A while OnEnter is pending");

            finish_enter(true);
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after OnEnter completes");
            ctx.Check(sm.GetCurrent() == "B", "Current state should become B after OnEnter completes");
        });

        add("Event during async OnExit rejected", [](TestContext& ctx) {
            Function<void(bool)> finish_exit;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](StateMachine&, Function<void(bool)> done) {
                finish_exit = pick(done);
            }});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});
            sm.AddTransition({"other", "A", "C"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while OnExit is pending");
            ctx.Check(!sm.TriggerEvent("other"), "Event during async OnExit should be rejected");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A during async OnExit");

            finish_exit(true);
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after OnExit completes");
            ctx.Check(sm.GetCurrent() == "B", "Current state should become B after OnExit completes");
        });

        add("Event during async OnEnter rejected", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                finish_enter = pick(done);
            }, {}});
            sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});
            sm.AddTransition({"other", "A", "C"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while OnEnter is pending");
            ctx.Check(!sm.TriggerEvent("other"), "Event during async OnEnter should be rejected");
            ctx.Check(sm.GetCurrent() == "A", "Current state should stay A during async OnEnter");

            finish_enter(true);
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after OnEnter completes");
            ctx.Check(sm.GetCurrent() == "B", "Current state should become B after OnEnter completes");
        });

        add("GoBack during async transition ignored", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                finish_enter = pick(done);
            }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while OnEnter is pending");
            sm.GoBack();
            ctx.Check(sm.GetCurrent() == "A", "GoBack() during async transition should be ignored");
            ctx.Check(sm.IsTransitioning(), "Machine should still be transitioning after ignored GoBack()");

            finish_enter(true);
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after OnEnter completes");
            ctx.Check(sm.GetCurrent() == "B", "Current state should become B after OnEnter completes");
        });

        add("Completing async OnExit with false aborts transition", [](TestContext& ctx) {
            Function<void(bool)> finish_exit;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](StateMachine&, Function<void(bool)> done) {
                finish_exit = pick(done);
            }});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            finish_exit(false);

            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnExit");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after failed OnExit");
            ctx.Check(sm.GetHistoryCount() == 1, "Failed OnExit should not add history");
        });

        add("Completing async OnEnter with false rolls back transition", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                finish_enter = pick(done);
            }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            finish_enter(false);

            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed OnEnter");
            ctx.Check(sm.GetCurrent() == "A", "Current state should roll back to A after failed OnEnter");
            ctx.Check(sm.GetHistoryCount() == 1, "Failed OnEnter should not add history");
        });

        add("Initial OnEnter double done ignored", [](TestContext& ctx) {
            Function<void(bool)> finish_start;
            int enter_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                ++enter_count;
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be started while initial OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while initial OnEnter is pending");

            finish_start(true);
            ctx.Check(sm.IsStarted(), "Machine should remain started after first done(true)");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after first done(true)");
            ctx.Check(sm.GetCurrent() == "A", "Current state should be A after first done(true)");

            finish_start(false);
            ctx.Check(sm.IsStarted(), "Machine should remain started after duplicate done()");
            ctx.Check(!sm.IsTransitioning(), "Duplicate done() should not re-open transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after duplicate done()");
            ctx.Check(enter_count == 1, "Initial OnEnter should run once");
            ctx.Check(!sm.CanGoBack(), "History should only contain the initial start record");
        });

        add("OnExit double done ignored", [](TestContext& ctx) {
            Function<void(bool)> finish_exit;
            int exit_count = 0;
            int after_count = 0;
            int finished_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](StateMachine&, Function<void(bool)> done) {
                ++exit_count;
                finish_exit = pick(done);
            }});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                ++finished_count;
            };

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [&](const TransitionContext&) {
                ++after_count;
            };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while OnExit is pending");

            finish_exit(true);
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B after first done(true)");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after first done(true)");
            ctx.Check(after_count == 1, "OnAfter should be called once after first done(true)");
            ctx.Check(finished_count == 1, "WhenTransitionFinished should be called once after first done(true)");

            finish_exit(false);
            ctx.Check(sm.GetCurrent() == "B", "Current state should stay B after duplicate done()");
            ctx.Check(!sm.IsTransitioning(), "Duplicate done() should not re-open transitioning");
            ctx.Check(after_count == 1, "OnAfter should not be called twice after duplicate done()");
            ctx.Check(finished_count == 1, "WhenTransitionFinished should not be called twice after duplicate done()");
            ctx.Check(exit_count == 1, "OnExit should be entered once");
        });

        add("OnEnter double done ignored", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;
            int enter_count = 0;
            int after_count = 0;
            int finished_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                ++enter_count;
                finish_enter = pick(done);
            }, {}});
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                ++finished_count;
            };

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [&](const TransitionContext&) {
                ++after_count;
            };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while OnEnter is pending");

            finish_enter(true);
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B after first done(true)");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after first done(true)");
            ctx.Check(after_count == 1, "OnAfter should be called once after first done(true)");
            ctx.Check(finished_count == 1, "WhenTransitionFinished should be called once after first done(true)");

            finish_enter(false);
            ctx.Check(sm.GetCurrent() == "B", "Current state should stay B after duplicate done()");
            ctx.Check(!sm.IsTransitioning(), "Duplicate done() should not re-open transitioning");
            ctx.Check(after_count == 1, "OnAfter should not be called twice after duplicate done()");
            ctx.Check(finished_count == 1, "WhenTransitionFinished should not be called twice after duplicate done()");
            ctx.Check(enter_count == 1, "OnEnter should be entered once");
        });

        add("OnAfter called once after double done", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;
            int after_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                finish_enter = pick(done);
            }, {}});

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [&](const TransitionContext&) {
                ++after_count;
            };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            finish_enter(true);
            finish_enter(false);

            ctx.Check(after_count == 1, "OnAfter should be called once after double done()");
        });

        add("WhenTransitionFinished called once after double done", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;
            int finished_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                finish_enter = pick(done);
            }, {}});
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                ++finished_count;
            };

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            finish_enter(true);
            finish_enter(false);

            ctx.Check(finished_count == 1, "WhenTransitionFinished should be called once after double done()");
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
                if (sm.TriggerEvent("go"))
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
            ctx.Check(rejected_count == 1000, "TriggerEvent() should begin 1,000 failed transitions");
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
