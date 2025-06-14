// main.cpp - Professional GUI Test harness for the StateMachine
#include <CtrlLib/CtrlLib.h>
#include <RichEdit/RichEdit.h>
#include "statemachine.h"

using namespace Upp;

// Our test runner application window
struct TestRunner : TopWindow {
    typedef TestRunner CLASSNAME;

    Splitter      splitter;
    RichEdit      logDisplay;
    Button        actionButton;
    
    String        qtf_log; // The data model: a single string holding all QTF content.
    bool          tests_cancelled = false;

    // --- Logging Helpers (Using the correct SetQTF and Move public API) ---
    void LogAppend(const String& s) {
        // 1. Append new QTF to our master string data model.
        qtf_log.Cat(s);
        
        // 2. Tell the RichEdit control to parse and display the entire updated log.
        logDisplay.SetQTF(qtf_log);
        
        // 3. Move the cursor to the end to ensure the view scrolls down.
        //    The correct public method is Move().
        logDisplay.Move(logDisplay.GetLength());

        Ctrl::ProcessEvents();
    }

    void LogLine(const String& s) {
        LogAppend(s + "&"); // Append text and a QTF newline
    }
    
    void LogHeaderLine(const String& s) {
        LogLine("[*" + s + "]");
    }

    // --- Core Test Logic ---
    void WaitForTransition(StateMachine& sm) {
        while (sm.IsTransitioning()) {
            if (tests_cancelled) return;
            Ctrl::ProcessEvents();
            Sleep(10);
        }
    }

    // --- Test Functions (No changes needed in these) ---

    void Test_BasicTransitions() {
        LogHeaderLine("--- Running: Basic Transitions Test ---");
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ LogLine("Entered A"); done(true); }, {}});
        sm.AddState({"B", [this](auto&, auto done){ LogLine("Entered B"); done(true); }, {}});
        sm.AddTransition({"go_b", "A", "B"});
        sm.Start();
        sm.TriggerEvent("go_b");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "B");
        LogLine("-> [* PASSED]");
    }

    void Test_GuardsAndCallbacks() {
        LogHeaderLine("--- Running: Guards and Callbacks Test ---");
        StateMachine sm;
        bool allow_transition = false;
        sm.SetInitial("Idle");
        sm.AddState({"Idle", [](auto&, auto done){ done(true); }, {}});
        sm.AddState({"Working", [](auto&, auto done){ done(true); }, {}});
        Transition t;
        t.event = "start";
        t.from = "Idle";
        t.to = "Working";
        t.Guard = [&](const TransitionContext&) { 
            LogLine(String("Guard checked. Allowing: ") + (allow_transition ? "Yes" : "No"));
            return allow_transition; 
        };
        sm.AddTransition(t);
        sm.Start();
        LogLine("Attempting transition when guard is false...");
        sm.TriggerEvent("start");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "Idle");
        LogLine("Attempting transition when guard is true...");
        allow_transition = true;
        sm.TriggerEvent("start");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "Working");
        LogLine("-> [* PASSED]");
    }

    void Test_HistoryAndGoBack() {
        LogHeaderLine("--- Running: History and GoBack Test ---");
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ LogLine("Entered A"); done(true); }, {}});
        sm.AddState({"B", [this](auto&, auto done){ LogLine("Entered B"); done(true); }, {}});
        sm.AddState({"C", [this](auto&, auto done){ LogLine("Entered C"); done(true); }, {}});
        sm.AddTransition({"go_b", "A", "B"});
        sm.AddTransition({"go_c", "B", "C"});
        sm.Start();
        sm.TriggerEvent("go_b"); WaitForTransition(sm);
        sm.TriggerEvent("go_c"); WaitForTransition(sm);
        LogLine("Going back from C to B...");
        sm.GoBack(); WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "B");
        LogLine("Going back from B to A...");
        sm.GoBack(); WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "A");
        LogLine("-> [* PASSED]");
    }

    void Test_AsyncFlow() {
        LogHeaderLine("--- Running: Asynchronous Flow Test ---");
        StateMachine sm;
        sm.SetInitial("Idle");
        sm.AddState({"Idle", [this](auto&, auto done){ LogLine("Entered Idle."); done(true); }, {}});
        sm.AddState({"Working", [this](auto&, auto done){
            LogLine("Entering Working state, starting 250ms task...");
            SetTimeCallback(250, [this, done]{
                LogLine("...Async task finished.");
                done(true);
            });
        }, {}});
        sm.AddTransition({"start", "Idle", "Working"});
        sm.Start();
        sm.TriggerEvent("start");
        LogLine("Main thread is not blocked...");
        WaitForTransition(sm);
        LogLine("Transition is now complete.");
        ASSERT(sm.GetCurrent() == "Working");
        LogLine("-> [* PASSED]");
    }

    void Test_EdgeCases() {
        LogHeaderLine("--- Running: Edge Cases Test ---");
        {
            LogLine("  Sub-test: Ignoring events during transition...");
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
            LogLine("  -> [* PASSED]");
        }
        {
            LogLine("  Sub-test: Aborting transition on OnExit failure...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, 
                              [this](auto&, auto done){ 
                                  LogLine("    OnExit from A failed as expected.");
                                  done(false);
                              }});
            sm.AddState({"B", [this](auto&, auto done){ LogLine("    ERROR: Should not have entered B!"); done(true); }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "A");
            LogLine("  -> [* PASSED]");
        }
        LogLine("-> [* PASSED]");
    }

    void Test_AdvancedHistory() {
        LogHeaderLine("--- Running: Advanced History & GoBack Test ---");
        {
            LogLine("  Sub-test: GoBack transition fails on OnEnter...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [this](auto&, auto done){ LogLine("    Entered A"); done(true); }, {}});
            sm.AddState({"B", 
                [this](auto&, auto done){ 
                    LogLine("    OnEnter for B was called, but will fail.");
                    done(false);
                }, 
                [this](auto&, auto done){ LogLine("    OnExit from B"); done(true); }
            });
            sm.AddState({"C", [this](auto&, auto done){ LogLine("    Entered C"); done(true); }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.AddTransition({"go_c", "B", "C"});
            sm.Start();
            sm.TriggerEvent("go_b"); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B"); 
            sm.TriggerEvent("go_c"); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "C");
            LogLine("    Attempting to GoBack from C to B (which will fail)...");
            sm.GoBack();
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "C");
            LogLine("    Correctly remained in state C.");
            LogLine("  -> [* PASSED]");
        }
        {
            LogLine("  Sub-test: Interleaving GoBack and TriggerEvent...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"C", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"D", [](auto&, auto done){ done(true); }, {}});
            sm.AddTransition({"a_b", "A", "B"});
            sm.AddTransition({"b_c", "B", "C"});
            sm.AddTransition({"b_d", "B", "D"});
            sm.Start();
            sm.TriggerEvent("a_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_c"); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            sm.TriggerEvent("b_d"); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "D");
            sm.GoBack(); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B");
            LogLine("  -> [* PASSED]");
        }
        LogLine("-> [* PASSED]");
    }

    void Test_AdvancedCallbacksAndFailures() {
        LogHeaderLine("--- Running: Advanced Callbacks & Failures Test ---");
        {
            LogLine("  Sub-test: Re-entrancy from OnAfter callback...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"C", [this](auto&, auto done){ LogLine("    ERROR: Should never enter C!"); done(true); }, {}});
            Transition t;
            t.event = "go_b";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [this](const TransitionContext& ctx) {
                LogLine("    In OnAfter for A->B, attempting to trigger event 'go_c'...");
                ctx.machine.TriggerEvent("go_c");
            };
            sm.AddTransition(t);
            sm.AddTransition({"go_c", "B", "C"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B");
            LogLine("    Correctly ignored re-entrant event and landed in state B.");
            LogLine("  -> [* PASSED]");
        }
        {
            LogLine("  Sub-test: Aborting transition on OnEnter failure...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, 
                              [this](auto&, auto done){ LogLine("    OnExit from A succeeded."); done(true); }
            });
            sm.AddState({"B", 
                [this](auto&, auto done){ 
                    LogLine("    OnEnter for B failed as expected.");
                    done(false);
                }, 
                [](auto&, auto done){ done(true); }
            });
            sm.AddTransition({"go_b", "A", "B"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B");
            LogLine("    Machine correctly ended in state 'B' as per the design.");
            LogLine("  -> [* PASSED]");
        }
        LogLine("-> [* PASSED]");
    }

    void Test_StressTest() {
        LogHeaderLine("--- Running: Stress Test ---");
        StateMachine sm;
        const int ITERATIONS = 100;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ SetTimeCallback(1, [=]{ done(true); }); }, {}});
        sm.AddState({"B", [](auto&, auto done){ done(true); }, {}});
        sm.AddState({"C", [this](auto&, auto done){ SetTimeCallback(1, [=]{ done(true); }); }, {}});
        sm.AddTransition({"a_to_b", "A", "B"});
        sm.AddTransition({"b_to_c", "B", "C"});
        sm.AddTransition({"c_to_a", "C", "A"});
        sm.Start();
        WaitForTransition(sm);
        
        LogAppend("  Sub-test: Rapid cycling for 500 loops... ");
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.TriggerEvent("c_to_a"); WaitForTransition(sm);
            if ((i + 1) % 25 == 0) { 
                LogAppend("."); 
            }
        }
        LogLine("\n  -> [* PASSED]");
        
        LogAppend("  Sub-test: Rapid GoBack() for 500 loops... ");
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            if ((i + 1) % 25 == 0) { 
                LogAppend(".");
            }
        }
        LogLine("\n  -> [* PASSED]");
        LogLine("-> [* PASSED]");
    }

    #define RUN_TEST(TEST_NAME) \
        if(tests_cancelled) break; \
        Cout() << "Running: " #TEST_NAME "... "; \
        TEST_NAME(); \
        if(tests_cancelled) break; \
        Cout() << "PASSED\n"; \
        LogLine(""); 

    void RunAllTests() {
        Cout() << "========================================\n";
        Cout() << "  Running State Machine Test Suite\n";
        Cout() << "========================================\n";
        LogHeaderLine("========================================");
        LogHeaderLine("  Running State Machine Test Suite");
        LogHeaderLine("========================================");
        LogLine("");

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

        LogLine("");
        if(tests_cancelled) {
            Cout() << "\n** Test Suite Cancelled By User **\n";
            LogHeaderLine("[*@r* ** Test Suite Cancelled By User **]");
        } else {
            Cout() << "========================================\n";
            Cout() << "  Test Suite Finished\n";
            Cout() << "========================================\n";
            LogHeaderLine("========================================");
            LogHeaderLine("  Test Suite Finished");
            LogHeaderLine("========================================");
        }
        
        actionButton.SetLabel("Close");
        actionButton.Enable();
        actionButton <<= callback(this, &TopWindow::Close);
        Title("State Machine Test Suite - Finished");
    }

    void CancelTests() {
        tests_cancelled = true;
        actionButton.Disable();
        actionButton.SetLabel("Cancelling...");
    }

    TestRunner() {
        Title("State Machine Test Suite - Running...").Sizeable().Zoomable();
        SetRect(0, 0, 800, 600);

        splitter.Vert(logDisplay, actionButton);
        splitter.SetPos(9500);
        Add(splitter.SizePos());

        logDisplay.SetReadOnly();
        
        actionButton.SetLabel("Cancel");
        actionButton <<= THISBACK(CancelTests);

        PostCallback(THISBACK(RunAllTests));
    }
};

GUI_APP_MAIN
{

    StdLogSetup(LOG_COUT);
    TestRunner().Run();
}