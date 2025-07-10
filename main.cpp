/**
 * main.cpp
 * A GUI test harness for the StateMachine library.
 *
 * This application provides a comprehensive test suite for the StateMachine class,
 * executing a series of validation tests in a user-friendly graphical interface.
 * It features a custom LogView control for detailed, color-coded output and allows
 * for real-time monitoring and cancellation of the test run.
 */

#include <CtrlLib/CtrlLib.h>
#include <RichEdit/RichEdit.h>
#include "statemachine.h"

using namespace Upp;

//------------------------------------------------------------------------------
// LogView: simple RichEdit wrapper for QTF-formatted, read-only logging
//------------------------------------------------------------------------------

struct LogView : RichEdit {
public:
    /// Defines the visual style for a log entry.
    enum LogStyle { LOG_NORMAL, LOG_HEADER, LOG_SUCCESS, LOG_ERROR };

    LogView() {
        SetReadOnly();
        NoRuler();
        ShowCodes(Null);
        qtf_log_buffer = "[A1] "; // Initialize with 8pt Arial font.
    }

    /**
     * Append one styled line (adds "&" if newline==true)
     */
    void Log(const String& text, LogStyle style = LOG_NORMAL, bool newline = true) {
        String qtf_text = text;
        switch(style) {
            case LOG_HEADER:  qtf_text = "[* "  + qtf_text + "]"; break;
            case LOG_SUCCESS: qtf_text = "[@g " + qtf_text + "]"; break;
            case LOG_ERROR:   qtf_text = "[*@r " + qtf_text + "]"; break;
            case LOG_NORMAL:
            default:
                break;
        }
        qtf_log_buffer.Cat(qtf_text);
        if(newline) qtf_log_buffer.Cat("&");

        SetQTF(qtf_log_buffer);
        Move(GetLength());
        Ctrl::ProcessEvents(); // Keep UI responsive during long tests.
    }

    /// Adds a horizontal rule to the log for visual separation.
    void AddSeparator() {
        Log("[--]", LOG_NORMAL);
    }

    /// Clears all content from the log view.
    void Clear() {
        qtf_log_buffer = "[A1] ";
        SetQTF(qtf_log_buffer);
    }

private:
    /// The internal QTF string that acts as the data model for the control.
    String qtf_log_buffer;
};

//------------------------------------------------------------------------------
// TestRunner: main window, orchestrates and displays test suite progress
//------------------------------------------------------------------------------

struct TestRunner : TopWindow {
    typedef TestRunner CLASSNAME;

    Splitter splitter;
    LogView  logDisplay;     ///< Shows detailed test output.
    Button   actionButton;   ///< Cancel/Close button.

    bool tests_cancelled = false; ///< Flag to gracefully stop the test suite.

    /**
     * Logs a simple message to the standard console output.
     */
    void ConsoleLog(const String& text) {
        Cout() << text;
    }

    /**
     * Pauses execution until the state machine is idle (not in a transition).
     */
    void WaitForTransition(StateMachine& sm) {
        while (sm.IsTransitioning()) {
            if (tests_cancelled) return;
            Ctrl::ProcessEvents();
            Sleep(10);
        }
    }

    // --- Test Suite ---
    // Each function below tests a specific feature of the StateMachine library.


    // Verifies the core functionality of state transitions.

    void Test_BasicTransitions() {
        logDisplay.Log("Running: Basic Transitions Test", LogView::LOG_HEADER);
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ logDisplay.Log("  Entered A"); done(true); }, {}});
        sm.AddState({"B", [this](auto&, auto done){ logDisplay.Log("  Entered B"); done(true); }, {}});
        sm.AddTransition({"go_b", "A", "B"});
        sm.Start();
        sm.TriggerEvent("go_b");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "B");
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    // Validates the behavior of transition guards and callbacks.
    void Test_GuardsAndCallbacks() {
        logDisplay.Log("Running: Guards and Callbacks Test", LogView::LOG_HEADER);
        StateMachine sm;
        bool allow_transition = false;
        sm.SetInitial("Idle");
        sm.AddState({"Idle",    [](auto&, auto done){ done(true); }, {}});
        sm.AddState({"Working", [](auto&, auto done){ done(true); }, {}});
        Transition t;
        t.event = "start";
        t.from  = "Idle";
        t.to    = "Working";
        t.Guard = [&](const TransitionContext&){
            logDisplay.Log(String("  Guard checked. Allowing: ") + (allow_transition ? "Yes" : "No"));
            return allow_transition;
        };
        sm.AddTransition(t);
        sm.Start();

        logDisplay.Log("  Attempting transition when guard is false...");
        sm.TriggerEvent("start");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "Idle");

        logDisplay.Log("  Attempting transition when guard is true...");
        allow_transition = true;
        sm.TriggerEvent("start");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "Working");

        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    // Tests the history and GoBack() functionality.
    void Test_HistoryAndGoBack() {
        logDisplay.Log("Running: History and GoBack Test", LogView::LOG_HEADER);
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ logDisplay.Log("  Entered A"); done(true); }, {}});
        sm.AddState({"B", [this](auto&, auto done){ logDisplay.Log("  Entered B"); done(true); }, {}});
        sm.AddState({"C", [this](auto&, auto done){ logDisplay.Log("  Entered C"); done(true); }, {}});
        sm.AddTransition({"go_b", "A", "B"});
        sm.AddTransition({"go_c", "B", "C"});
        sm.Start();

        sm.TriggerEvent("go_b"); WaitForTransition(sm);
        sm.TriggerEvent("go_c"); WaitForTransition(sm);

        logDisplay.Log("  Going back from C to B...");
        sm.GoBack(); WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "B");

        logDisplay.Log("  Going back from B to A...");
        sm.GoBack(); WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "A");

        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    // Verifies asynchronous flow within state handlers.
    void Test_AsyncFlow() {
        logDisplay.Log("Running: Asynchronous Flow Test", LogView::LOG_HEADER);
        StateMachine sm;
        sm.SetInitial("Idle");
        sm.AddState({"Idle", [this](auto&, auto done){ logDisplay.Log("  Entered Idle."); done(true); }, {}});
        sm.AddState({"Working", [this](auto&, auto done){
            logDisplay.Log("  Entering Working state, starting 250ms task...");
            SetTimeCallback(250, [this, done]{
                logDisplay.Log("  ...Async task finished.");
                done(true);
            });
        }, {}});
        sm.AddTransition({"start", "Idle", "Working"});
        sm.Start();

        sm.TriggerEvent("start");
        logDisplay.Log("  Main thread is not blocked...");
        WaitForTransition(sm);
        logDisplay.Log("  Transition is now complete.");
        ASSERT(sm.GetCurrent() == "Working");
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    // Probes edge cases, including ignored events and failing OnExit.
    void Test_EdgeCases() {
        logDisplay.Log("Running: Edge Cases Test", LogView::LOG_HEADER);
        {
            logDisplay.Log("  Sub-test: Ignoring events during transition...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"B", [this](auto&, auto done){ SetTimeCallback(100, [=]{ done(true); }); }, {}});
            sm.AddState({"C", [](auto&, auto done){ done(true); }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.AddTransition({"go_c", "A", "C"});
            sm.Start();
            sm.TriggerEvent("go_b");
            sm.TriggerEvent("go_c");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
        {
            logDisplay.Log("  Sub-test: Aborting transition on OnExit failure...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A",
                [](auto&, auto done){ done(true); },
                [this](auto&, auto done){
                    logDisplay.Log("    OnExit from A failed as expected.");
                    done(false);
                }
            });
            sm.AddState({"B", [this](auto&, auto done){
                    logDisplay.Log("    ERROR: Should not have entered B!", LogView::LOG_ERROR);
                    done(true);
                }, {}
            });
            sm.AddTransition({"go_b", "A", "B"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "A");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    // Runs advanced history & GoBack scenarios.
    void Test_AdvancedHistory() {
        logDisplay.Log("Running: Advanced History & GoBack Test", LogView::LOG_HEADER);

        // Sub-test 1: GoBack fails on OnEnter
        {
            logDisplay.Log("  Sub-test: GoBack transition fails on OnEnter...");
            LOG("=== Sub-test 1: GoBack transition fails on OnEnter ===");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [this](auto&, auto done){
                logDisplay.Log("    Entered A");
                LOG("OnEnter A called");
                done(true);
            }, {}});
            sm.AddState({"B",
                [this](auto&, auto done){
                    logDisplay.Log("    OnEnter for B was called, but will fail.");
                    LOG("OnEnter B called (will fail)");
                    done(false);
                },
                [this](auto&, auto done){
                    logDisplay.Log("    OnExit from B");
                    LOG("OnExit B called");
                    done(true);
                }
            });
            sm.AddState({"C", [this](auto&, auto done){
                logDisplay.Log("    Entered C");
                LOG("OnEnter C called");
                done(true);
            }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.AddTransition({"go_c", "B", "C"});
            sm.Start();

            LOG("After Start");
            sm.DumpHistory();
            sm.TriggerEvent("go_b"); WaitForTransition(sm);
            LOG("After go_b");
            sm.DumpHistory();
            ASSERT(sm.GetCurrent() == "A");

            sm.TriggerEvent("go_c"); WaitForTransition(sm);
            LOG("After go_c");
            sm.DumpHistory();
            ASSERT(sm.GetCurrent() == "A");

            logDisplay.Log("    Attempting GoBack from A (should do nothing)...");
            LOG("Attempting GoBack from A");
            sm.GoBack(); WaitForTransition(sm);
            LOG("After GoBack");
            sm.DumpHistory();
            ASSERT(sm.GetCurrent() == "A");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }

        // Sub-test 2: Interleaving GoBack and TriggerEvent
        {
            logDisplay.Log("  Sub-test: Interleaving GoBack and TriggerEvent...");
            LOG("=== Sub-test 2: Interleaving GoBack and TriggerEvent ===");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ LOG("OnEnter A"); done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done){ LOG("OnEnter B"); done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done){ LOG("OnEnter C"); done(true); }, {}});
            sm.AddState({"D", [](auto&, auto done){ LOG("OnEnter D"); done(true); }, {}});
            sm.AddTransition({"a_b", "A", "B"});
            sm.AddTransition({"b_c", "B", "C"});
            sm.AddTransition({"b_d", "B", "D"});
            sm.Start();

            LOG("After Start");
            sm.DumpHistory();
            sm.TriggerEvent("a_b"); WaitForTransition(sm);
            LOG("After a_b");
            sm.DumpHistory();
            sm.TriggerEvent("b_c"); WaitForTransition(sm);
            LOG("After b_c");
            sm.DumpHistory();
            sm.GoBack(); WaitForTransition(sm);
            LOG("After GoBack");
            sm.DumpHistory();
            sm.TriggerEvent("b_d"); WaitForTransition(sm);
            LOG("After b_d");
            sm.DumpHistory();
            ASSERT(sm.GetCurrent() == "D");
            sm.GoBack(); WaitForTransition(sm);
            LOG("After GoBack");
            sm.DumpHistory();
            ASSERT(sm.GetCurrent() == "B");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }

        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
        LOG("=== Test_AdvancedHistory finished ===");
    }

    // Tests complex callback and failure scenarios.
    void Test_AdvancedCallbacksAndFailures() {
        logDisplay.Log("Running: Advanced Callbacks & Failures Test", LogView::LOG_HEADER);
        {
            logDisplay.Log("  Sub-test: Aborting transition on OnEnter failure...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A",
                [](auto&, auto done){ done(true); },
                [this](auto&, auto done){ logDisplay.Log("    OnExit from A succeeded."); done(true); }
            });
            sm.AddState({"B",
                [this](auto&, auto done){
                    logDisplay.Log("    OnEnter for B failed as expected.");
                    done(false);
                },
                [](auto&, auto done){ done(true); }
            });
            sm.AddTransition({"go_b", "A", "B"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);

            ASSERT(sm.GetCurrent() == "A");
            logDisplay.Log("    Machine correctly remained in state 'A' after failed transition.");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
    }

    // Puts the state machine under high load to test stability.
    void Test_StressTest() {
        logDisplay.Log("Running: Stress Test", LogView::LOG_HEADER);
        StateMachine sm;
        const int ITERATIONS = 10;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ SetTimeCallback(1, [=]{ done(true); }); }, {}});
        sm.AddState({"B", [](auto&, auto done){ done(true); }, {}});
        sm.AddState({"C", [this](auto&, auto done){ SetTimeCallback(1, [=]{ done(true); }); }, {}});
        sm.AddTransition({"a_to_b", "A", "B"});
        sm.AddTransition({"b_to_c", "B", "C"});
        sm.AddTransition({"c_to_a", "C", "A"});
        sm.Start();
        WaitForTransition(sm);

        logDisplay.Log("  Sub-test: Rapid cycling... ", LogView::LOG_NORMAL, false);
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.TriggerEvent("c_to_a"); WaitForTransition(sm);
            if ((i + 1) % 5 == 0)
                logDisplay.Log(".", LogView::LOG_NORMAL, false);
        }
        logDisplay.Log(" done.", LogView::LOG_NORMAL);

        logDisplay.Log("  Sub-test: Rapid GoBack()... ", LogView::LOG_NORMAL, false);
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            if ((i + 1) % 5 == 0)
                logDisplay.Log(".", LogView::LOG_NORMAL, false);
        }
        logDisplay.Log(" done.", LogView::LOG_NORMAL);
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    /// A macro to simplify running a test and logging its console status.
    #define RUN_TEST(TEST_NAME)                \
        if(tests_cancelled) break;            \
        ConsoleLog("Running: " #TEST_NAME "\n"); \
        TEST_NAME();                          \
        if(tests_cancelled) break;            \
        ConsoleLog("PASSED\n");               \
        logDisplay.AddSeparator();

    // Executes all defined test functions in sequence.
    void RunAllTests() {
        ConsoleLog("========================================\n");
        ConsoleLog("  Running State Machine Test Suite\n");
        ConsoleLog("========================================\n");
        logDisplay.Log("State Machine Test Suite", LogView::LOG_HEADER);
        logDisplay.Log("");

        do {
            RUN_TEST(Test_BasicTransitions);
            RUN_TEST(Test_GuardsAndCallbacks);
            RUN_TEST(Test_HistoryAndGoBack);
            RUN_TEST(Test_AsyncFlow);
            RUN_TEST(Test_EdgeCases);
            RUN_TEST(Test_AdvancedHistory);
            RUN_TEST(Test_AdvancedCallbacksAndFailures);
            RUN_TEST(Test_StressTest);
        } while(0);

        logDisplay.Log("");
        if (tests_cancelled) {
            ConsoleLog("\n** Test Suite Cancelled By User **\n");
            logDisplay.Log("Test Suite Cancelled By User", LogView::LOG_ERROR);
        }
        else {
            ConsoleLog("========================================\n");
            ConsoleLog("  Test Suite Finished\n");
            ConsoleLog("========================================\n");
            logDisplay.Log("Test Suite Finished", LogView::LOG_SUCCESS);
        }

        actionButton.SetLabel("Close");
        actionButton.Enable();
        actionButton <<= callback(this, &TopWindow::Close);
        Title("State Machine Test Suite - Finished");
    }

    // Sets the cancellation flag and updates the GUI.
    void CancelTests() {
        tests_cancelled = true;
        actionButton.Disable();
        actionButton.SetLabel("Cancelling...");
    }

    // Constructor for the main application window.
    TestRunner() {
        Title("State Machine Test Suite - Running...").Sizeable().Zoomable();
        SetRect(0, 0, 800, 600);

        splitter.Vert(logDisplay, actionButton);
        splitter.SetPos(9500);
        Add(splitter.SizePos());

        actionButton.SetLabel("Cancel");
        actionButton <<= THISBACK(CancelTests);

        // Schedule the test suite to run after window creation.
        PostCallback(THISBACK(RunAllTests));
    }
};

// The main entry point for the U++ GUI application.
GUI_APP_MAIN {
    StdLogSetup(LOG_COUT);
    TestRunner().Run();
}

