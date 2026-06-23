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

struct PhaseSnapshot : Moveable<PhaseSnapshot> {
    String phase;
    String current;
    bool started = false;
    bool transitioning = false;
    int history = 0;
    StateMachineError last_error = StateMachineError::None;
};

struct ReentryOutcome {
    String before_current;
    String before_initial;
    bool before_started = false;
    bool before_transitioning = false;
    int before_history = 0;
    int before_states = 0;
    int before_transitions = 0;
    StateMachineError before_last_error = StateMachineError::None;

    bool trigger_ok = true;
    StateMachineError trigger_error = StateMachineError::None;
    bool try_ok = true;
    StateMachineError try_error = StateMachineError::None;
    bool go_ok = true;
    StateMachineError go_error = StateMachineError::None;
    bool reset_ok = true;
    StateMachineError reset_error = StateMachineError::None;
    bool clear_ok = true;
    StateMachineError clear_error = StateMachineError::None;
    bool add_state_ok = true;
    StateMachineError add_state_error = StateMachineError::None;
    bool add_transition_ok = true;
    StateMachineError add_transition_error = StateMachineError::None;
    bool set_initial_ok = true;
    StateMachineError set_initial_error = StateMachineError::None;
    String current;
    String initial;
    bool started = false;
    bool transitioning = false;
    int history = 0;
    int states = 0;
    int transitions = 0;
    StateMachineError last_error = StateMachineError::None;

    String after_current;
    String after_initial;
    bool after_started = false;
    bool after_transitioning = false;
    int after_history = 0;
    int after_states = 0;
    int after_transitions = 0;
    StateMachineError after_last_error = StateMachineError::None;
};

static ReentryOutcome CaptureReentry(StateMachine& sm) {
    ReentryOutcome out;

    out.before_current = sm.GetCurrent();
    out.before_initial = sm.GetInitial();
    out.before_started = sm.IsStarted();
    out.before_transitioning = sm.IsTransitioning();
    out.before_history = sm.GetHistoryCount();
    out.before_states = sm.GetStateCount();
    out.before_transitions = sm.GetTransitionCount();
    out.before_last_error = sm.GetLastError();

    Transition direct;
    direct.event = "go";
    direct.from = "A";
    direct.to = "B";

    out.trigger_ok = sm.TriggerEvent("go");
    out.trigger_error = sm.GetLastError();

    out.try_ok = sm.TryTransition(direct);
    out.try_error = sm.GetLastError();

    out.go_ok = sm.GoBack();
    out.go_error = sm.GetLastError();

    out.reset_ok = sm.Reset();
    out.reset_error = sm.GetLastError();

    out.clear_ok = sm.Clear();
    out.clear_error = sm.GetLastError();

    out.add_state_ok = sm.AddState({"Temp", {}, {}});
    out.add_state_error = sm.GetLastError();

    out.add_transition_ok = sm.AddTransition({"temp", "A", "Temp"});
    out.add_transition_error = sm.GetLastError();

    out.set_initial_ok = sm.SetInitial("Temp");
    out.set_initial_error = sm.GetLastError();

    out.current = sm.GetCurrent();
    out.initial = sm.GetInitial();
    out.started = sm.IsStarted();
    out.transitioning = sm.IsTransitioning();
    out.history = sm.GetHistoryCount();
    out.states = sm.GetStateCount();
    out.transitions = sm.GetTransitionCount();
    out.last_error = sm.GetLastError();

    out.after_current = out.current;
    out.after_initial = out.initial;
    out.after_started = out.started;
    out.after_transitioning = out.transitioning;
    out.after_history = out.history;
    out.after_states = out.states;
    out.after_transitions = out.transitions;
    out.after_last_error = out.last_error;
    return out;
}

static void CheckReentryStateStable(TestContext& ctx, const ReentryOutcome& out, const String& phase) {
    ctx.Check(out.before_current == out.after_current, phase + ": current changed");
    ctx.Check(out.before_initial == out.after_initial, phase + ": initial changed");
    ctx.Check(out.before_started == out.after_started, phase + ": started changed");
    ctx.Check(out.before_transitioning == out.after_transitioning, phase + ": transitioning changed");
    ctx.Check(out.before_history == out.after_history, phase + ": history changed");
    ctx.Check(out.before_states == out.after_states, phase + ": state count changed");
    ctx.Check(out.before_transitions == out.after_transitions, phase + ": transition count changed");
}

static void VerifyReentryOutcome(TestContext& ctx, const ReentryOutcome& out, const String& phase, const String& expected_current, int expected_history) {
    CheckReentryStateStable(ctx, out, phase);
    ctx.Check(out.before_current == expected_current, phase + ": current before calls should stay stable");
    ctx.Check(out.after_current == expected_current, phase + ": current after calls should stay stable");
    ctx.Check(out.before_initial == "A", phase + ": initial before calls should stay A");
    ctx.Check(out.after_initial == "A", phase + ": initial after calls should stay A");
    ctx.Check(out.before_started, phase + ": machine should be started before reentrant calls");
    ctx.Check(out.after_started, phase + ": machine should remain started after reentrant calls");
    ctx.Check(out.before_transitioning, phase + ": machine should be transitioning before reentrant calls");
    ctx.Check(out.after_transitioning, phase + ": machine should remain transitioning after reentrant calls");
    ctx.Check(out.before_history == expected_history, phase + ": history before calls should match phase");
    ctx.Check(out.after_history == expected_history, phase + ": history after calls should remain unchanged");
    ctx.Check(out.before_states == 2 && out.after_states == 2, phase + ": state count should remain 2");
    ctx.Check(out.before_transitions == 1 && out.after_transitions == 1, phase + ": transition count should remain 1");
    ctx.Check(out.before_last_error == StateMachineError::None, phase + ": last error should be clear before reentrant calls");

    ctx.Check(!out.trigger_ok, phase + ": TriggerEvent() should return false");
    ctx.Check(out.trigger_error == StateMachineError::EventRejectedWhileTransitioning, phase + ": TriggerEvent() should set EventRejectedWhileTransitioning");
    ctx.Check(!out.try_ok, phase + ": TryTransition() should return false");
    ctx.Check(out.try_error == StateMachineError::TransitionInProgress, phase + ": TryTransition() should set TransitionInProgress");
    ctx.Check(!out.go_ok, phase + ": GoBack() should return false");
    ctx.Check(out.go_error == StateMachineError::TransitionInProgress, phase + ": GoBack() should set TransitionInProgress");
    ctx.Check(!out.reset_ok, phase + ": Reset() should return false");
    ctx.Check(out.reset_error == StateMachineError::TransitionInProgress, phase + ": Reset() should set TransitionInProgress");
    ctx.Check(!out.clear_ok, phase + ": Clear() should return false");
    ctx.Check(out.clear_error == StateMachineError::TransitionInProgress, phase + ": Clear() should set TransitionInProgress");
    ctx.Check(!out.add_state_ok, phase + ": AddState() should return false");
    ctx.Check(out.add_state_error == StateMachineError::AlreadyStarted, phase + ": AddState() should set AlreadyStarted");
    ctx.Check(!out.add_transition_ok, phase + ": AddTransition() should return false");
    ctx.Check(out.add_transition_error == StateMachineError::AlreadyStarted, phase + ": AddTransition() should set AlreadyStarted");
    ctx.Check(!out.set_initial_ok, phase + ": SetInitial() should return false");
    ctx.Check(out.set_initial_error == StateMachineError::AlreadyStarted, phase + ": SetInitial() should set AlreadyStarted");
    ctx.Check(out.last_error == StateMachineError::AlreadyStarted, phase + ": final last error should reflect the last rejected config call");
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

        add("Default event policy is RejectWhileTransitioning", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.GetEventPolicy() == EventPolicy::RejectWhileTransitioning, "Default event policy should be RejectWhileTransitioning");
        });

        add("SetEventPolicy RejectWhileTransitioning", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetEventPolicy(EventPolicy::RejectWhileTransitioning);
            ctx.Check(sm.GetEventPolicy() == EventPolicy::RejectWhileTransitioning, "Stored event policy should be RejectWhileTransitioning");
        });

        add("SetEventPolicy DropWhileTransitioning", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetEventPolicy(EventPolicy::DropWhileTransitioning);
            ctx.Check(sm.GetEventPolicy() == EventPolicy::DropWhileTransitioning, "Stored event policy should be DropWhileTransitioning");
        });

        add("SetEventPolicy QueueWhileTransitioning", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetEventPolicy(EventPolicy::QueueWhileTransitioning);
            ctx.Check(sm.GetEventPolicy() == EventPolicy::QueueWhileTransitioning, "Stored event policy should be QueueWhileTransitioning");
        });

        add("QueueWhileTransitioning currently rejects until implemented", [](TestContext& ctx) {
            Function<void(bool)> finish_exit;

            StateMachine sm;
            sm.SetInitial("A");
            sm.SetEventPolicy(EventPolicy::QueueWhileTransitioning);
            ctx.Check(sm.AddState({"A", {}, [&](StateMachine&, Function<void(bool)> done) {
                finish_exit = pick(done);
            }}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "TriggerEvent() should begin");
            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should still reject while transitioning");
            ctx.Check(sm.GetLastError() == StateMachineError::EventQueueingNotImplemented, "Last error should be EventQueueingNotImplemented");
            finish_exit(true);
        });

        add("DropWhileTransitioning drops event during async transition", [](TestContext& ctx) {
            Function<void(bool)> finish_exit;

            StateMachine sm;
            sm.SetInitial("A");
            sm.SetEventPolicy(EventPolicy::DropWhileTransitioning);
            ctx.Check(sm.AddState({"A", {}, [&](StateMachine&, Function<void(bool)> done) {
                finish_exit = pick(done);
            }}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "TriggerEvent() should begin");
            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should be dropped while transitioning");
            ctx.Check(sm.GetLastError() == StateMachineError::EventDroppedWhileTransitioning, "Last error should be EventDroppedWhileTransitioning");
            finish_exit(true);
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

        add("SetInitial valid id accepted", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.SetInitial("A"), "SetInitial() should return true");
        });

        add("SetInitial empty id rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(!sm.SetInitial(""), "SetInitial() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::EmptyStateId, "Last error should be EmptyStateId");
        });

        add("SetInitial after Start rejected", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.SetInitial("A"), "SetInitial() should return true");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.SetInitial("B"), "SetInitial() should return false after Start()");
            ctx.Check(sm.GetLastError() == StateMachineError::AlreadyStarted, "Last error should be AlreadyStarted");
        });

        add("GetInitial returns configured id", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.SetInitial("A"), "SetInitial() should return true");
            ctx.Check(sm.GetInitial() == "A", "GetInitial() should return A");
        });

        add("HasInitial false before SetInitial", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(!sm.HasInitial(), "HasInitial() should return false");
        });

        add("HasInitial true after SetInitial", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.SetInitial("A"), "SetInitial() should return true");
            ctx.Check(sm.HasInitial(), "HasInitial() should return true");
        });

        add("HasState finds existing state", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.HasState("A"), "HasState() should find A");
        });

        add("HasState rejects missing state", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(!sm.HasState("Missing"), "HasState() should not find missing state");
        });

        add("HasTransition finds existing transition", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.HasTransition("A", "go"), "HasTransition() should find A/go");
        });

        add("HasTransition rejects missing transition", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(!sm.HasTransition("B", "go"), "HasTransition() should not find B/go");
        });

        add("Counts update after AddState/AddTransition/Clear", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.GetStateCount() == 0, "Initial state count should be 0");
            ctx.Check(sm.GetTransitionCount() == 0, "Initial transition count should be 0");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.GetStateCount() == 2, "State count should be 2 after AddState()");
            ctx.Check(sm.GetTransitionCount() == 1, "Transition count should be 1 after AddTransition()");
            ctx.Check(sm.Clear(), "Clear() should return true");
            ctx.Check(sm.GetStateCount() == 0, "State count should be 0 after Clear()");
            ctx.Check(sm.GetTransitionCount() == 0, "Transition count should be 0 after Clear()");
        });
    });

    RunGroup("Error API", passed, failed, [&](auto add) {
        add("AddState duplicate sets DuplicateStateId", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "First AddState() should return true");
            ctx.Check(!sm.AddState({"A", {}, {}}), "Duplicate AddState() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::DuplicateStateId, "Last error should be DuplicateStateId");
            ctx.Check(sm.GetLastErrorText() == "Duplicate state id", "Last error text should be stable");
        });

        add("AddTransition missing target sets MissingToState", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(!sm.AddTransition({"go", "A", "B"}), "AddTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::MissingToState, "Last error should be MissingToState");
        });

        add("Start missing initial sets MissingState", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("Missing");
            ctx.Check(!sm.Start(), "Start() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::MissingState, "Last error should be MissingState");
        });

        add("Start OnEnter failure sets StartEnterFailed", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}}), "State A should be added");

            ctx.Check(sm.Start(), "Start() should return true");
            finish_start(false);
            ctx.Check(sm.GetLastError() == StateMachineError::StartEnterFailed, "Last error should be StartEnterFailed");
            ctx.Check(sm.GetLastErrorText() == "Start enter failed", "Last error text should describe the start failure");
        });

        add("TriggerEvent unknown event sets NoMatchingTransition", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.TriggerEvent("missing"), "TriggerEvent() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::NoMatchingTransition, "Last error should be NoMatchingTransition");
        });

        add("TriggerEvent guard false sets GuardRejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.Guard = [](const TransitionContext&) { return false; };
            ctx.Check(sm.AddTransition(t), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::GuardRejected, "Last error should be GuardRejected");
        });

        add("Transition OnExit failure sets ExitFailed", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, [](auto&, auto done) { done(false); } }), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "TriggerEvent() should begin");
            ctx.Check(sm.GetLastError() == StateMachineError::ExitFailed, "Last error should be ExitFailed");
        });

        add("Transition OnEnter failure sets EnterFailed", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(false); }, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "TriggerEvent() should begin");
            ctx.Check(sm.GetLastError() == StateMachineError::EnterFailed, "Last error should be EnterFailed");
        });

        add("TryTransition wrong source sets WrongSourceState", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "B";
            t.to = "A";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::WrongSourceState, "Last error should be WrongSourceState");
        });

        add("TryTransition empty event rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.from = "A";
            t.to = "B";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::EmptyEvent, "Last error should be EmptyEvent");
        });

        add("TryTransition empty from rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.to = "B";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::EmptyFromState, "Last error should be EmptyFromState");
        });

        add("TryTransition empty to rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "A";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::EmptyToState, "Last error should be EmptyToState");
        });

        add("TryTransition missing from rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "Missing";
            t.to = "B";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::MissingFromState, "Last error should be MissingFromState");
        });

        add("TryTransition missing to rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "Missing";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::MissingToState, "Last error should be MissingToState");
        });

        add("TryTransition wrong current source rejected", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");

            Transition t;
            t.event = "go";
            t.from = "B";
            t.to = "A";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetLastError() == StateMachineError::WrongSourceState, "Last error should be WrongSourceState");
        });

        add("Successful call clears last error", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(!sm.AddState({"A", {}, {}}), "Duplicate AddState() should fail");
            ctx.Check(sm.GetLastError() == StateMachineError::DuplicateStateId, "Last error should be DuplicateStateId before success");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "AddTransition() should succeed");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Successful AddTransition() should clear the last error");
        });

        add("GoBack OnExit failure sets BackTransitionFailed", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(true); }, {}}), "State B should be added");
            ctx.Check(sm.AddState({"C", [](auto&, auto done) { done(true); }, [](auto&, auto done) { done(false); } }), "State C should be added");
            ctx.Check(sm.AddTransition({"to_b", "A", "B"}), "Transition A->B should be added");
            ctx.Check(sm.AddTransition({"to_c", "B", "C"}), "Transition B->C should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.TriggerEvent("to_c"), "B->C should begin");
            ctx.Check(sm.GoBack(), "GoBack() should return true");
            ctx.Check(sm.GetLastError() == StateMachineError::BackTransitionFailed, "Last error should be BackTransitionFailed");
        });

        add("GoBack OnEnter failure sets BackTransitionFailed", [](TestContext& ctx) {
            bool allow_b_enter = true;

            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                done(allow_b_enter);
            }, {}}), "State B should be added");
            ctx.Check(sm.AddState({"C", [](auto&, auto done) { done(true); }, {}}), "State C should be added");
            ctx.Check(sm.AddTransition({"to_b", "A", "B"}), "Transition A->B should be added");
            ctx.Check(sm.AddTransition({"to_c", "B", "C"}), "Transition B->C should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.TriggerEvent("to_c"), "B->C should begin");
            allow_b_enter = false;
            ctx.Check(sm.GoBack(), "GoBack() should return true");
            ctx.Check(sm.GetLastError() == StateMachineError::BackTransitionFailed, "Last error should be BackTransitionFailed");
        });
    });

    RunGroup("Lifecycle control", passed, failed, [&](auto add) {
        add("Reset clears current state", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B before Reset()");

            ctx.Check(sm.Reset(), "Reset() should return true");
            ctx.Check(sm.GetCurrent().IsEmpty(), "Reset() should clear current");
            ctx.Check(!sm.IsStarted(), "Reset() should clear started");
            ctx.Check(!sm.IsTransitioning(), "Reset() should clear transitioning");
        });

        add("Reset clears history", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetHistoryCount() == 2, "History should have two entries before Reset()");

            ctx.Check(sm.Reset(), "Reset() should return true");
            ctx.Check(sm.GetHistoryCount() == 0, "Reset() should clear history");
            ctx.Check(!sm.CanGoBack(), "Reset() should leave no history to go back to");
        });

        add("Reset allows Start again", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.Reset(), "Reset() should return true");
            ctx.Check(sm.Start(), "Start() should work again after Reset()");
            ctx.Check(sm.GetCurrent() == "A", "Current state should be A after restarting");
        });

        add("Reset keeps states and transitions", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.Reset(), "Reset() should return true");
            ctx.Check(sm.Start(), "Start() should work again after Reset()");
            ctx.Check(sm.TriggerEvent("go"), "Transition should still work after Reset()");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B after reused configuration");
        });

        add("Reset during transition is rejected", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}}), "State A should be added");

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning during async Start()");
            ctx.Check(!sm.Reset(), "Reset() should return false while transitioning");
            ctx.Check(sm.IsStarted(), "Reset() rejection should not clear started");
            ctx.Check(sm.IsTransitioning(), "Reset() rejection should not clear transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Reset() rejection should not clear current");
            finish_start(true);
            ctx.Check(sm.IsStarted(), "Machine should still be started after async completion");
        });

        add("Clear removes states and transitions", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Clear(), "Clear() should return true");
            ctx.Check(sm.GetCurrent().IsEmpty(), "Clear() should clear current");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Clear() should clear last error");
            ctx.Check(!sm.Start(), "Start() should fail after Clear() removed configuration");
        });

        add("Clear clears initial state", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.Clear(), "Clear() should return true");
            ctx.Check(!sm.Start(), "Start() should fail after Clear() removed initial state");
        });

        add("Clear allows fresh configuration", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Clear(), "Clear() should return true");

            sm.SetInitial("X");
            ctx.Check(sm.AddState({"X", {}, {}}), "Fresh state X should be added");
            ctx.Check(sm.AddState({"Y", {}, {}}), "Fresh state Y should be added");
            ctx.Check(sm.AddTransition({"step", "X", "Y"}), "Fresh transition should be added");
            ctx.Check(sm.Start(), "Start() should work after fresh configuration");
            ctx.Check(sm.TriggerEvent("step"), "Fresh transition should work after Clear()");
            ctx.Check(sm.GetCurrent() == "Y", "Current state should be Y after fresh configuration");
        });

        add("Clear during transition rejected", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}}), "State A should be added");

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning during async Start()");
            ctx.Check(!sm.Clear(), "Clear() should return false while transitioning");
            ctx.Check(sm.IsStarted(), "Clear() rejection should not clear started");
            ctx.Check(sm.IsTransitioning(), "Clear() rejection should not clear transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Clear() rejection should not clear current");
            finish_start(true);
            ctx.Check(sm.IsStarted(), "Machine should still be started after async completion");
        });
    });

    RunGroup("Consistency", passed, failed, [&](auto add) {
        add("Reset after failed transition leaves config reusable", [](TestContext& ctx) {
            StateMachine sm;
            bool fail_exit = true;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](auto&, auto done) { done(!fail_exit); } }), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "TriggerEvent() should begin");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after failed transition");
            ctx.Check(sm.Reset(), "Reset() should return true after failure");
            fail_exit = false;
            ctx.Check(sm.Start(), "Start() should work again after Reset()");
            ctx.Check(sm.TriggerEvent("go"), "Transition should work again after Reset()");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B after reused configuration");
        });

        add("Clear after Reset leaves machine empty", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.Reset(), "Reset() should return true");
            ctx.Check(sm.Clear(), "Clear() should return true after Reset()");
            ctx.Check(!sm.HasInitial(), "Clear() should remove initial state");
            ctx.Check(sm.GetStateCount() == 0, "Clear() should remove states");
            ctx.Check(sm.GetTransitionCount() == 0, "Clear() should remove transitions");
            ctx.Check(!sm.Start(), "Start() should fail after Clear()");
        });

        add("Last error clears after successful operation", [](TestContext& ctx) {
            StateMachine sm;
            ctx.Check(!sm.SetInitial(""), "SetInitial() should fail for empty id");
            ctx.Check(sm.GetLastError() == StateMachineError::EmptyStateId, "Last error should be EmptyStateId");
            ctx.Check(sm.SetInitial("A"), "SetInitial() should succeed");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Successful SetInitial() should clear the last error");
        });

        add("History unchanged after rejected TriggerEvent", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            const int history_before = sm.GetHistoryCount();
            ctx.Check(!sm.TriggerEvent("missing"), "TriggerEvent() should return false");
            ctx.Check(sm.GetHistoryCount() == history_before, "History should be unchanged after rejected TriggerEvent()");
        });

        add("History unchanged after guard rejection", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.Guard = [](const TransitionContext&) { return false; };
            ctx.Check(sm.AddTransition(t), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            const int history_before = sm.GetHistoryCount();
            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should return false");
            ctx.Check(sm.GetHistoryCount() == history_before, "History should be unchanged after guard rejection");
        });

        add("History unchanged after failed OnExit", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, [](auto&, auto done) { done(false); }}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            const int history_before = sm.GetHistoryCount();
            ctx.Check(sm.TriggerEvent("go"), "TriggerEvent() should begin");
            ctx.Check(sm.GetHistoryCount() == history_before, "History should be unchanged after failed OnExit()");
        });

        add("History unchanged after failed OnEnter", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", [](auto&, auto done) { done(false); }, {}}), "State B should be added");
            ctx.Check(sm.AddTransition({"go", "A", "B"}), "Transition should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            const int history_before = sm.GetHistoryCount();
            ctx.Check(sm.TriggerEvent("go"), "TriggerEvent() should begin");
            ctx.Check(sm.GetHistoryCount() == history_before, "History should be unchanged after failed OnEnter()");
        });

        add("Current state unchanged after rejected TryTransition", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.AddState({"B", {}, {}}), "State B should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            Transition t;
            t.event = "go";
            t.from = "B";
            t.to = "A";
            ctx.Check(!sm.TryTransition(t), "TryTransition() should return false");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after rejected TryTransition()");
        });

        add("Current state unchanged after rejected GoBack", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            ctx.Check(sm.AddState({"A", {}, {}}), "State A should be added");
            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.GoBack(), "GoBack() should return false with no history");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after rejected GoBack()");
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

        add("GoBack before Start returns false", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(!sm.GoBack(), "GoBack() should return false");
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

        add("RejectWhileTransitioning rejects event during async transition", [](TestContext& ctx) {
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
            ctx.Check(sm.GetLastError() == StateMachineError::EventRejectedWhileTransitioning, "Last error should be EventRejectedWhileTransitioning");

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
            ctx.Check(sm.GoBack(), "GoBack() should return true");
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
            ctx.Check(!sm.GoBack(), "GoBack() at the start should return false");
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

        add("GoBack from B returns true", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"to_b", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("to_b"), "A->B should begin");
            ctx.Check(sm.GetCurrent() == "B", "Current state should be B before GoBack()");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true before GoBack()");
            ctx.Check(sm.GoBack(), "GoBack() should return true");
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
            ctx.Check(sm.GoBack(), "GoBack() should return true");
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
            ctx.Check(sm.GoBack(), "First GoBack() should return true");
            ctx.Check(sm.GetCurrent() == "B", "First GoBack() should return to B");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should still be true after first GoBack()");
            ctx.Check(sm.GoBack(), "Second GoBack() should return true");
            ctx.Check(sm.GetCurrent() == "A", "Second GoBack() should return to A");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after returning to A");
        });

        add("GoBack at start returns false", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false at the start");
            ctx.Check(!sm.GoBack(), "GoBack() should return false at the start");
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
            ctx.Check(sm.GoBack(), "GoBack() should return true");
            ctx.Check(sm.GetCurrent() == "A", "GoBack() should return to A without a reverse transition");
            ctx.Check(!sm.CanGoBack(), "CanGoBack() should be false after returning to A");
        });

        add("GoBack failed OnExit returns true but keeps current/history", [](TestContext& ctx) {
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
            ctx.Check(sm.GoBack(), "GoBack() should return true");
            ctx.Check(sm.GetCurrent() == "C", "Failed GoBack() OnExit should keep current state C");
            ctx.Check(sm.GetHistoryCount() == 3, "Failed GoBack() OnExit should keep history unchanged");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should remain true after failed GoBack()");
            ctx.Check(!sm.IsTransitioning(), "Machine should not remain transitioning after failed GoBack()");
        });

        add("GoBack failed OnEnter returns true but keeps current/history", [](TestContext& ctx) {
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
            ctx.Check(sm.GoBack(), "GoBack() should return true");
            ctx.Check(sm.GetCurrent() == "C", "Failed GoBack() OnEnter should keep current state C");
            ctx.Check(sm.GetHistoryCount() == 3, "Failed GoBack() OnEnter should keep history unchanged");
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
            ctx.Check(sm.GoBack(), "GoBack() should return true");
            ctx.Check(sm.GetCurrent() == "B", "GoBack() should return to B before branching");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should still be true after returning to B");
            ctx.Check(sm.TriggerEvent("to_d"), "B->D should begin on the new branch");
            ctx.Check(sm.GetCurrent() == "D", "Current state should be D after branching");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should be true after branching");
            ctx.Check(sm.GoBack(), "Branch GoBack() should return true");
            ctx.Check(sm.GetCurrent() == "B", "GoBack() from the branch should return to B");
            ctx.Check(sm.CanGoBack(), "CanGoBack() should still be true after returning to B");
            ctx.Check(sm.GoBack(), "Second branch GoBack() should return true");
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

        add("Callback phases observe source during OnEnter", [](TestContext& ctx) {
            StateMachine sm;
            Vector<PhaseSnapshot> snapshots;
            auto capture = [&](const String& phase) {
                PhaseSnapshot snap;
                snap.phase = phase;
                snap.current = sm.GetCurrent();
                snap.started = sm.IsStarted();
                snap.transitioning = sm.IsTransitioning();
                snap.history = sm.GetHistoryCount();
                snap.last_error = sm.GetLastError();
                snapshots.Add(snap);
            };

            sm.SetInitial("Idle");
            sm.AddState({
                "Idle",
                [&](auto&, auto done) {
                    capture("Idle.OnEnter");
                    done(true);
                },
                [&](auto&, auto done) {
                    capture("Idle.OnExit");
                    done(true);
                }
            });
            sm.AddState({
                "Working",
                [&](auto&, auto done) {
                    capture("Working.OnEnter");
                    done(true);
                },
                {}
            });

            Transition t;
            t.event = "start";
            t.from = "Idle";
            t.to = "Working";
            t.Guard = [&](const TransitionContext&) {
                capture("Guard");
                return true;
            };
            t.OnBefore = [&](const TransitionContext&) { capture("OnBefore"); };
            t.OnAfter = [&](const TransitionContext&) { capture("OnAfter"); };
            sm.WhenTransitionStarted = [&](const TransitionContext&) {
                capture("WhenTransitionStarted");
            };
            sm.WhenTransitionFinished = [&](const TransitionContext&) {
                capture("WhenTransitionFinished");
            };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            snapshots.Clear();

            ctx.Check(sm.TriggerEvent("start"), "Transition should begin");
            capture("After completion");

            ctx.Check(snapshots.GetCount() == 8, "Should capture all transition phases plus completion");

            auto find = [&](const String& phase) -> const PhaseSnapshot* {
                for (const auto& snap : snapshots)
                    if (snap.phase == phase)
                        return &snap;
                return nullptr;
            };

            const PhaseSnapshot* guard = find("Guard");
            const PhaseSnapshot* started = find("WhenTransitionStarted");
            const PhaseSnapshot* before = find("OnBefore");
            const PhaseSnapshot* exit = find("Idle.OnExit");
            const PhaseSnapshot* enter = find("Working.OnEnter");
            const PhaseSnapshot* finished = find("WhenTransitionFinished");
            const PhaseSnapshot* after = find("OnAfter");
            const PhaseSnapshot* complete = find("After completion");

            ctx.Check(guard, "Guard snapshot should exist");
            ctx.Check(started, "WhenTransitionStarted snapshot should exist");
            ctx.Check(before, "OnBefore snapshot should exist");
            ctx.Check(exit, "OnExit snapshot should exist");
            ctx.Check(enter, "OnEnter snapshot should exist");
            ctx.Check(finished, "WhenTransitionFinished snapshot should exist");
            ctx.Check(after, "OnAfter snapshot should exist");
            ctx.Check(complete, "Completion snapshot should exist");

            auto check_snapshot = [&](const PhaseSnapshot& snap, const String& current, bool started_flag, bool transitioning_flag, int history, StateMachineError error, const String& label) {
                ctx.Check(snap.current == current, label + ": current state mismatch");
                ctx.Check(snap.started == started_flag, label + ": started flag mismatch");
                ctx.Check(snap.transitioning == transitioning_flag, label + ": transitioning flag mismatch");
                ctx.Check(snap.history == history, label + ": history count mismatch");
                ctx.Check(snap.last_error == error, label + ": last error mismatch");
            };

            check_snapshot(*guard, "Idle", true, false, 1, StateMachineError::None, "Guard");
            check_snapshot(*started, "Idle", true, true, 1, StateMachineError::None, "WhenTransitionStarted");
            check_snapshot(*before, "Idle", true, true, 1, StateMachineError::None, "OnBefore");
            check_snapshot(*exit, "Idle", true, true, 1, StateMachineError::None, "OnExit");
            check_snapshot(*enter, "Idle", true, true, 1, StateMachineError::None, "OnEnter");
            check_snapshot(*finished, "Working", true, true, 2, StateMachineError::None, "WhenTransitionFinished");
            check_snapshot(*after, "Working", true, true, 2, StateMachineError::None, "OnAfter");
            check_snapshot(*complete, "Working", true, false, 2, StateMachineError::None, "After completion");
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
            ctx.Check(sm.GetLastError() == StateMachineError::EventRejectedWhileTransitioning, "Last error should be EventRejectedWhileTransitioning");

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
            ctx.Check(sm.GetLastError() == StateMachineError::EventRejectedWhileTransitioning, "Last error should be EventRejectedWhileTransitioning");

            finish_enter(true);
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after OnEnter completes");
            ctx.Check(sm.GetCurrent() == "B", "Current state should become B after OnEnter completes");
        });

        add("GoBack while transitioning returns false", [](TestContext& ctx) {
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
            ctx.Check(!sm.GoBack(), "GoBack() should return false while transitioning");
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

    RunGroup("Reentrancy", passed, failed, [&](auto add) {
        add("OnBefore rejects public calls without corrupting state", [&](TestContext& ctx) {
            ReentryOutcome out;
            StateMachine sm;
            sm.SetEventPolicy(EventPolicy::RejectWhileTransitioning);
            sm.SetInitial("A");
            sm.AddState({
                "A",
                [](auto&, auto done) { done(true); },
                [](auto&, auto done) { done(true); }
            });
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnBefore = [&](const TransitionContext&) {
                out = CaptureReentry(sm);
            };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            VerifyReentryOutcome(ctx, out, "OnBefore", "A", 1);
            ctx.Check(sm.GetCurrent() == "B", "Current state should still complete to B");
            ctx.Check(sm.GetHistoryCount() == 2, "History should remain committed after transition");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after callback chain completes");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Successful transition should clear last error");
        });

        add("OnExit rejects public calls without corrupting state", [&](TestContext& ctx) {
            ReentryOutcome out;
            StateMachine sm;
            sm.SetEventPolicy(EventPolicy::RejectWhileTransitioning);
            sm.SetInitial("A");
            sm.AddState({
                "A",
                [](auto&, auto done) { done(true); },
                [&](auto&, auto done) {
                    out = CaptureReentry(sm);
                    done(true);
                }
            });
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            VerifyReentryOutcome(ctx, out, "OnExit", "A", 1);
            ctx.Check(sm.GetCurrent() == "B", "Current state should still complete to B");
            ctx.Check(sm.GetHistoryCount() == 2, "History should remain committed after transition");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after callback chain completes");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Successful transition should clear last error");
        });

        add("OnEnter rejects public calls without corrupting state", [&](TestContext& ctx) {
            ReentryOutcome out;
            StateMachine sm;
            sm.SetEventPolicy(EventPolicy::RejectWhileTransitioning);
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({
                "B",
                [&](auto&, auto done) {
                    out = CaptureReentry(sm);
                    done(true);
                },
                {}
            });
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            VerifyReentryOutcome(ctx, out, "OnEnter", "A", 1);
            ctx.Check(sm.GetCurrent() == "B", "Current state should still complete to B");
            ctx.Check(sm.GetHistoryCount() == 2, "History should remain committed after transition");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after callback chain completes");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Successful transition should clear last error");
        });

        add("OnAfter rejects public calls without corrupting state", [&](TestContext& ctx) {
            ReentryOutcome out;
            StateMachine sm;
            sm.SetEventPolicy(EventPolicy::RejectWhileTransitioning);
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [&](const TransitionContext&) {
                out = CaptureReentry(sm);
            };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            VerifyReentryOutcome(ctx, out, "OnAfter", "B", 2);
            ctx.Check(sm.GetCurrent() == "B", "Current state should still complete to B");
            ctx.Check(sm.GetHistoryCount() == 2, "History should remain committed after transition");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after callback chain completes");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Successful transition should clear last error");
        });
    });

    RunGroup("Async edge cases", passed, failed, [&](auto add) {
        auto run_start_order_case = [&](TestContext& ctx, const String& label, bool first_result, bool second_result) {
            Function<void(bool)> finish_start;
            int enter_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                ++enter_count;
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), label + ": Start() should return true");
            ctx.Check(sm.IsStarted(), label + ": machine should be started while initial OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), label + ": machine should be transitioning while initial OnEnter is pending");

            finish_start(first_result);
            if (first_result) {
                ctx.Check(sm.IsStarted(), label + ": machine should stay started after done(true)");
                ctx.Check(!sm.IsTransitioning(), label + ": machine should stop transitioning after done(true)");
                ctx.Check(sm.GetCurrent() == "A", label + ": current state should be A after done(true)");
                ctx.Check(sm.GetHistoryCount() == 1, label + ": history should contain one __start record after done(true)");
                ctx.Check(sm.GetLastError() == StateMachineError::None, label + ": successful done(true) should clear the error");
            }
            else {
                ctx.Check(!sm.IsStarted(), label + ": machine should roll back after done(false)");
                ctx.Check(!sm.IsTransitioning(), label + ": machine should stop transitioning after done(false)");
                ctx.Check(sm.GetCurrent().IsEmpty(), label + ": current state should be cleared after done(false)");
                ctx.Check(sm.GetHistoryCount() == 0, label + ": failed startup should not record history");
                ctx.Check(sm.GetLastError() == StateMachineError::StartEnterFailed, label + ": failed startup should set StartEnterFailed");
            }

            finish_start(second_result);
            if (first_result) {
                ctx.Check(sm.IsStarted(), label + ": duplicate done() should keep the machine started");
                ctx.Check(!sm.IsTransitioning(), label + ": duplicate done() should not reopen transitioning");
                ctx.Check(sm.GetCurrent() == "A", label + ": duplicate done() should keep current A");
                ctx.Check(sm.GetHistoryCount() == 1, label + ": duplicate done() should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::None, label + ": duplicate done() should not change the final error");
            }
            else {
                ctx.Check(!sm.IsStarted(), label + ": duplicate done() should keep the machine stopped");
                ctx.Check(!sm.IsTransitioning(), label + ": duplicate done() should keep transitioning false");
                ctx.Check(sm.GetCurrent().IsEmpty(), label + ": duplicate done() should keep current cleared");
                ctx.Check(sm.GetHistoryCount() == 0, label + ": duplicate done() should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::StartEnterFailed, label + ": duplicate done() should not change the final error");
            }

            ctx.Check(enter_count == 1, label + ": initial OnEnter should run once");
        };

        add("Initial OnEnter completion orders are single-shot", [&](TestContext& ctx) {
            run_start_order_case(ctx, "done(true) then done(true)", true, true);
            run_start_order_case(ctx, "done(false) then done(false)", false, false);
            run_start_order_case(ctx, "done(false) then done(true)", false, true);
            run_start_order_case(ctx, "done(true) then done(false)", true, false);
        });

        auto run_exit_order_case = [&](TestContext& ctx, const String& label, bool first_result, bool second_result) {
            Function<void(bool)> finish_exit;
            int exit_count = 0;
            int enter_count = 0;
            int after_count = 0;
            int finished_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](StateMachine&, Function<void(bool)> done) {
                ++exit_count;
                finish_exit = pick(done);
            }});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                ++enter_count;
                done(true);
            }, {}});
            sm.WhenTransitionFinished = [&](const TransitionContext&) { ++finished_count; };

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [&](const TransitionContext&) { ++after_count; };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), label + ": Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), label + ": transition should begin");
            ctx.Check(sm.IsTransitioning(), label + ": machine should be transitioning while OnExit is pending");

            finish_exit(first_result);
            if (first_result) {
                ctx.Check(sm.IsStarted(), label + ": machine should remain started after done(true)");
                ctx.Check(!sm.IsTransitioning(), label + ": machine should stop transitioning after done(true)");
                ctx.Check(sm.GetCurrent() == "B", label + ": current state should be B after done(true)");
                ctx.Check(sm.GetHistoryCount() == 2, label + ": history should contain the completed transition after done(true)");
                ctx.Check(sm.GetLastError() == StateMachineError::None, label + ": successful done(true) should clear the error");
                ctx.Check(enter_count == 1, label + ": target OnEnter should run once after done(true)");
                ctx.Check(after_count == 1, label + ": OnAfter should run once after done(true)");
                ctx.Check(finished_count == 1, label + ": WhenTransitionFinished should run once after done(true)");
            }
            else {
                ctx.Check(sm.IsStarted(), label + ": machine should remain started after done(false)");
                ctx.Check(!sm.IsTransitioning(), label + ": machine should stop transitioning after done(false)");
                ctx.Check(sm.GetCurrent() == "A", label + ": current state should remain A after done(false)");
                ctx.Check(sm.GetHistoryCount() == 1, label + ": failed exit should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::ExitFailed, label + ": failed exit should set ExitFailed");
                ctx.Check(enter_count == 0, label + ": target OnEnter should not run after failed exit");
                ctx.Check(after_count == 0, label + ": OnAfter should not run after failed exit");
                ctx.Check(finished_count == 0, label + ": WhenTransitionFinished should not run after failed exit");
            }

            finish_exit(second_result);
            if (first_result) {
                ctx.Check(sm.GetCurrent() == "B", label + ": duplicate done() should keep current B");
                ctx.Check(sm.GetHistoryCount() == 2, label + ": duplicate done() should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::None, label + ": duplicate done() should not change the final error");
                ctx.Check(enter_count == 1, label + ": target OnEnter should still run once");
                ctx.Check(after_count == 1, label + ": OnAfter should still run once");
                ctx.Check(finished_count == 1, label + ": WhenTransitionFinished should still run once");
            }
            else {
                ctx.Check(sm.GetCurrent() == "A", label + ": duplicate done() should keep current A");
                ctx.Check(sm.GetHistoryCount() == 1, label + ": duplicate done() should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::ExitFailed, label + ": duplicate done() should not change the final error");
                ctx.Check(enter_count == 0, label + ": target OnEnter should still not run");
                ctx.Check(after_count == 0, label + ": OnAfter should still not run");
                ctx.Check(finished_count == 0, label + ": WhenTransitionFinished should still not run");
            }

            ctx.Check(!sm.IsTransitioning(), label + ": duplicate done() should keep transitioning false");
            ctx.Check(exit_count == 1, label + ": source OnExit should run once");
        };

        add("OnExit completion orders are single-shot", [&](TestContext& ctx) {
            run_exit_order_case(ctx, "done(true) then done(true)", true, true);
            run_exit_order_case(ctx, "done(false) then done(false)", false, false);
            run_exit_order_case(ctx, "done(false) then done(true)", false, true);
            run_exit_order_case(ctx, "done(true) then done(false)", true, false);
        });

        auto run_enter_order_case = [&](TestContext& ctx, const String& label, bool first_result, bool second_result) {
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
            sm.WhenTransitionFinished = [&](const TransitionContext&) { ++finished_count; };

            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [&](const TransitionContext&) { ++after_count; };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), label + ": Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), label + ": transition should begin");
            ctx.Check(sm.IsTransitioning(), label + ": machine should be transitioning while OnEnter is pending");

            finish_enter(first_result);
            if (first_result) {
                ctx.Check(sm.IsStarted(), label + ": machine should remain started after done(true)");
                ctx.Check(!sm.IsTransitioning(), label + ": machine should stop transitioning after done(true)");
                ctx.Check(sm.GetCurrent() == "B", label + ": current state should be B after done(true)");
                ctx.Check(sm.GetHistoryCount() == 2, label + ": history should contain the completed transition after done(true)");
                ctx.Check(sm.GetLastError() == StateMachineError::None, label + ": successful done(true) should clear the error");
                ctx.Check(after_count == 1, label + ": OnAfter should run once after done(true)");
                ctx.Check(finished_count == 1, label + ": WhenTransitionFinished should run once after done(true)");
            }
            else {
                ctx.Check(sm.IsStarted(), label + ": machine should remain started after done(false)");
                ctx.Check(!sm.IsTransitioning(), label + ": machine should stop transitioning after done(false)");
                ctx.Check(sm.GetCurrent() == "A", label + ": current state should remain A after done(false)");
                ctx.Check(sm.GetHistoryCount() == 1, label + ": failed enter should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::EnterFailed, label + ": failed enter should set EnterFailed");
                ctx.Check(after_count == 0, label + ": OnAfter should not run after failed enter");
                ctx.Check(finished_count == 0, label + ": WhenTransitionFinished should not run after failed enter");
            }

            finish_enter(second_result);
            if (first_result) {
                ctx.Check(sm.GetCurrent() == "B", label + ": duplicate done() should keep current B");
                ctx.Check(sm.GetHistoryCount() == 2, label + ": duplicate done() should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::None, label + ": duplicate done() should not change the final error");
                ctx.Check(after_count == 1, label + ": OnAfter should still run once");
                ctx.Check(finished_count == 1, label + ": WhenTransitionFinished should still run once");
            }
            else {
                ctx.Check(sm.GetCurrent() == "A", label + ": duplicate done() should keep current A");
                ctx.Check(sm.GetHistoryCount() == 1, label + ": duplicate done() should not add history");
                ctx.Check(sm.GetLastError() == StateMachineError::EnterFailed, label + ": duplicate done() should not change the final error");
                ctx.Check(after_count == 0, label + ": OnAfter should still not run");
                ctx.Check(finished_count == 0, label + ": WhenTransitionFinished should still not run");
            }

            ctx.Check(!sm.IsTransitioning(), label + ": duplicate done() should keep transitioning false");
            ctx.Check(enter_count == 1, label + ": target OnEnter should run once");
        };

        add("OnEnter completion orders are single-shot", [&](TestContext& ctx) {
            run_enter_order_case(ctx, "done(true) then done(true)", true, true);
            run_enter_order_case(ctx, "done(false) then done(false)", false, false);
            run_enter_order_case(ctx, "done(false) then done(true)", false, true);
            run_enter_order_case(ctx, "done(true) then done(false)", true, false);
        });

        add("Async exit failure skips target OnEnter", [](TestContext& ctx) {
            Function<void(bool)> finish_exit;
            int exit_count = 0;
            int enter_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](StateMachine&, Function<void(bool)> done) {
                ++exit_count;
                finish_exit = pick(done);
            }});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                ++enter_count;
                done(true);
            }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            finish_exit(false);

            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed async OnExit");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after failed async OnExit");
            ctx.Check(sm.GetHistoryCount() == 1, "Failed async OnExit should not add history");
            ctx.Check(sm.GetLastError() == StateMachineError::ExitFailed, "Failed async OnExit should set ExitFailed");
            ctx.Check(exit_count == 1, "OnExit should run once");
            ctx.Check(enter_count == 0, "OnEnter should not run when async OnExit fails");
        });

        add("Async enter failure rolls back current state", [](TestContext& ctx) {
            Function<void(bool)> finish_enter;
            int exit_count = 0;
            int enter_count = 0;
            int after_count = 0;
            int finished_count = 0;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](StateMachine&, Function<void(bool)> done) {
                ++exit_count;
                done(true);
            }});
            sm.AddState({"B", [&](StateMachine&, Function<void(bool)> done) {
                ++enter_count;
                finish_enter = pick(done);
            }, {}});
            sm.WhenTransitionFinished = [&](const TransitionContext&) { ++finished_count; };
            Transition t;
            t.event = "go";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [&](const TransitionContext&) { ++after_count; };
            sm.AddTransition(t);

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            finish_enter(false);

            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after failed async OnEnter");
            ctx.Check(sm.GetCurrent() == "A", "Current state should roll back to A after failed async OnEnter");
            ctx.Check(sm.GetHistoryCount() == 1, "Failed async OnEnter should not add history");
            ctx.Check(sm.GetLastError() == StateMachineError::EnterFailed, "Failed async OnEnter should set EnterFailed");
            ctx.Check(exit_count == 1, "OnExit should run once");
            ctx.Check(enter_count == 1, "OnEnter should run once");
            ctx.Check(after_count == 0, "OnAfter should not run when async OnEnter fails");
            ctx.Check(finished_count == 0, "WhenTransitionFinished should not run when async OnEnter fails");
        });
    });

    RunGroup("Startup edge cases", passed, failed, [&](auto add) {
        add("Async startup follows event policy", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.SetEventPolicy(EventPolicy::DropWhileTransitioning);
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be started while initial OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while initial OnEnter is pending");
            ctx.Check(!sm.TriggerEvent("go"), "TriggerEvent() should be rejected while startup is pending");
            ctx.Check(sm.GetLastError() == StateMachineError::EventDroppedWhileTransitioning, "Startup policy should drop events while transitioning");

            finish_start(true);
            ctx.Check(sm.IsStarted(), "Machine should remain started after startup completes");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after startup completes");
            ctx.Check(sm.GetCurrent() == "A", "Current state should remain A after startup completes");
        });

        add("Reset and Clear are rejected during async startup", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be started while initial OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while initial OnEnter is pending");
            ctx.Check(!sm.Reset(), "Reset() should be rejected during async startup");
            ctx.Check(!sm.Clear(), "Clear() should be rejected during async startup");
            ctx.Check(sm.IsStarted(), "Rejected runtime calls should not clear started");
            ctx.Check(sm.IsTransitioning(), "Rejected runtime calls should not clear transitioning");
            ctx.Check(sm.GetCurrent() == "A", "Rejected runtime calls should not clear current");

            finish_start(true);
            ctx.Check(sm.IsStarted(), "Machine should remain started after startup completes");
            ctx.Check(!sm.IsTransitioning(), "Machine should stop transitioning after startup completes");
        });

        add("Failed async startup rolls back runtime and history", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.IsStarted(), "Machine should be started while initial OnEnter is pending");
            ctx.Check(sm.IsTransitioning(), "Machine should be transitioning while initial OnEnter is pending");
            finish_start(false);

            ctx.Check(!sm.IsStarted(), "Failed async startup should clear started");
            ctx.Check(!sm.IsTransitioning(), "Failed async startup should clear transitioning");
            ctx.Check(sm.GetCurrent().IsEmpty(), "Failed async startup should clear current");
            ctx.Check(sm.GetHistoryCount() == 0, "Failed async startup should clear history");
            ctx.Check(sm.GetLastError() == StateMachineError::StartEnterFailed, "Failed async startup should set StartEnterFailed");
        });

        add("Startup double completion does not duplicate history", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            finish_start(true);
            ctx.Check(sm.GetHistoryCount() == 1, "Successful startup should create exactly one history record");
            ctx.Check(sm.GetHistoryEvent(0) == "__start", "Successful startup history event should be __start");
            finish_start(false);
            ctx.Check(sm.GetHistoryCount() == 1, "Duplicate startup completion should not add history");
            ctx.Check(sm.GetCurrent() == "A", "Duplicate startup completion should not change current");
            ctx.Check(sm.GetLastError() == StateMachineError::None, "Duplicate startup completion should not change the final error");
        });
    });

    RunGroup("History invariants", passed, failed, [&](auto add) {
        add("Successful startup creates exactly one __start record", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.GetHistoryCount() == 1, "Start() should create one history record");
            ctx.Check(sm.GetHistoryFrom(0).IsEmpty(), "Start history from-state should be empty");
            ctx.Check(sm.GetHistoryTo(0) == "A", "Start history to-state should be A");
            ctx.Check(sm.GetHistoryEvent(0) == "__start", "Start history event should be __start");
        });

        add("Failed startup creates no history", [](TestContext& ctx) {
            Function<void(bool)> finish_start;

            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [&](StateMachine&, Function<void(bool)> done) {
                finish_start = pick(done);
            }, {}});

            ctx.Check(sm.Start(), "Start() should return true");
            finish_start(false);
            ctx.Check(sm.GetHistoryCount() == 0, "Failed startup should create no history");
            ctx.Check(!sm.CanGoBack(), "Failed startup should leave no history to go back to");
        });

        add("Successful transition appends exactly one history entry", [](TestContext& ctx) {
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
            sm.AddTransition({"go", "A", "B"});

            ctx.Check(sm.Start(), "Start() should return true");
            ctx.Check(sm.TriggerEvent("go"), "Transition should begin");
            ctx.Check(sm.GetHistoryCount() == 2, "Successful transition should append one history entry");
            ctx.Check(sm.GetHistoryFrom(1) == "A", "History from-state should be A");
            ctx.Check(sm.GetHistoryTo(1) == "B", "History to-state should be B");
            ctx.Check(sm.GetHistoryEvent(1) == "go", "History event should be go");
        });

        add("Rejected or failed transitions do not add history", [](TestContext& ctx) {
            {
                StateMachine sm;
                sm.SetInitial("A");
                sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
                sm.AddState({"C", [](auto&, auto done) { done(true); }, {}});
                Transition guard_blocked;
                guard_blocked.event = "guard";
                guard_blocked.from = "A";
                guard_blocked.to = "C";
                guard_blocked.Guard = [](const TransitionContext&) { return false; };
                sm.AddTransition(guard_blocked);

                ctx.Check(sm.Start(), "Guard rejection: Start() should return true");
                const int history_before = sm.GetHistoryCount();
                ctx.Check(!sm.TriggerEvent("guard"), "Guard rejection should return false");
                ctx.Check(sm.GetHistoryCount() == history_before, "Guard rejection should not add history");
            }

            {
                StateMachine sm;
                sm.SetInitial("A");
                sm.AddState({"A", [](auto&, auto done) { done(true); }, [&](auto&, auto done) { done(false); }});
                sm.AddState({"B", [](auto&, auto done) { done(true); }, {}});
                sm.AddTransition({"exit_fail", "A", "B"});

                ctx.Check(sm.Start(), "Exit failure: Start() should return true");
                const int history_before = sm.GetHistoryCount();
                ctx.Check(sm.TriggerEvent("exit_fail"), "Failed OnExit should begin");
                ctx.Check(sm.GetHistoryCount() == history_before, "Failed OnExit should not add history");
            }

            {
                StateMachine sm;
                sm.SetInitial("A");
                sm.AddState({"A", [](auto&, auto done) { done(true); }, {}});
                sm.AddState({"B", [](auto&, auto done) { done(false); }, {}});
                sm.AddTransition({"enter_fail", "A", "B"});

                ctx.Check(sm.Start(), "Enter failure: Start() should return true");
                const int history_before = sm.GetHistoryCount();
                ctx.Check(sm.TriggerEvent("enter_fail"), "Failed OnEnter should begin");
                ctx.Check(sm.GetHistoryCount() == history_before, "Failed OnEnter should not add history");
            }
        });

        add("GoBack, Reset, and Clear keep history consistent", [](TestContext& ctx) {
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
            ctx.Check(sm.GetHistoryCount() == 3, "History should contain start plus two transitions");
            ctx.Check(sm.GoBack(), "GoBack() should return true");
            ctx.Check(sm.GetHistoryCount() == 2, "GoBack() should pop one history entry");
            ctx.Check(sm.GetHistoryTo(1) == "B", "Remaining top history entry should point to B");
            ctx.Check(sm.Reset(), "Reset() should return true");
            ctx.Check(sm.GetHistoryCount() == 0, "Reset() should clear history");
            ctx.Check(sm.AddState({"D", [](auto&, auto done) { done(true); }, {}}), "AddState() should still work after Reset()");
            ctx.Check(sm.Clear(), "Clear() should return true");
            ctx.Check(sm.GetHistoryCount() == 0, "Clear() should leave history empty");
            ctx.Check(!sm.HasInitial(), "Clear() should remove the configured initial state");
            ctx.Check(sm.GetStateCount() == 0, "Clear() should remove states");
            ctx.Check(sm.GetTransitionCount() == 0, "Clear() should remove transitions");
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
                ctx.Check(sm.GoBack(), "First GoBack() in stress run should return true");
                ++go_back_count;
                if (sm.GetCurrent() != "B")
                    ok = false;
                if (!sm.CanGoBack())
                    ok = false;
                ctx.Check(sm.GoBack(), "Second GoBack() in stress run should return true");
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
