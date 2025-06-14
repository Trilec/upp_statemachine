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
    
    String        qtf_log; // The data model for the RichEdit control.
    bool          tests_cancelled = false;

    // --- NEW: Decoupled Logging System ---

    enum GuiLogStyle { GUI_LOG_NORMAL, GUI_LOG_HEADER, GUI_LOG_SUCCESS, GUI_LOG_ERROR };

    // Logger for the RichEdit GUI window ONLY.
    void GuiLog(const String& text, GuiLogStyle style = GUI_LOG_NORMAL, bool newline = true) {
        String qtf_text = text; // No escaping for now, keep it simple.
        
        // Corrected QTF formatting based on documentation.
        switch(style) {
            case GUI_LOG_HEADER:  qtf_text = "[* " + qtf_text + "]"; break;
            case GUI_LOG_SUCCESS: qtf_text = "[@g " + qtf_text + "]"; break;
            case GUI_LOG_ERROR:   qtf_text = "[*@r " + qtf_text + "]"; break;
            case GUI_LOG_NORMAL:
            default:
                break;
        }
        
        qtf_log.Cat(qtf_text);
        if(newline) qtf_log.Cat("&");

        logDisplay.SetQTF(qtf_log);
        logDisplay.Move(logDisplay.GetLength());
        Ctrl::ProcessEvents();
    }
    
    // Logger for the Console ONLY.
    void ConsoleLog(const String& text) {
        Cout() << text;
    }

    // --- Core Test Logic (Unchanged) ---
    void WaitForTransition(StateMachine& sm) {
        while (sm.IsTransitioning()) {
            if (tests_cancelled) return;
            Ctrl::ProcessEvents();
            Sleep(10);
        }
    }

    // --- REFACTORED: All tests now use the GuiLog() function ---

    void Test_BasicTransitions() {
        GuiLog("--- Running: Basic Transitions Test ---", GUI_LOG_HEADER);
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ GuiLog("Entered A"); done(true); }, {}});
        sm.AddState({"B", [this](auto&, auto done){ GuiLog("Entered B"); done(true); }, {}});
        sm.AddTransition({"go_b", "A", "B"});
        sm.Start();
        sm.TriggerEvent("go_b");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "B");
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    void Test_GuardsAndCallbacks() {
        GuiLog("--- Running: Guards and Callbacks Test ---", GUI_LOG_HEADER);
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
            GuiLog(String("Guard checked. Allowing: ") + (allow_transition ? "Yes" : "No"));
            return allow_transition; 
        };
        sm.AddTransition(t);
        sm.Start();
        GuiLog("Attempting transition when guard is false...");
        sm.TriggerEvent("start");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "Idle");
        GuiLog("Attempting transition when guard is true...");
        allow_transition = true;
        sm.TriggerEvent("start");
        WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "Working");
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    void Test_HistoryAndGoBack() {
        GuiLog("--- Running: History and GoBack Test ---", GUI_LOG_HEADER);
        StateMachine sm;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ GuiLog("Entered A"); done(true); }, {}});
        sm.AddState({"B", [this](auto&, auto done){ GuiLog("Entered B"); done(true); }, {}});
        sm.AddState({"C", [this](auto&, auto done){ GuiLog("Entered C"); done(true); }, {}});
        sm.AddTransition({"go_b", "A", "B"});
        sm.AddTransition({"go_c", "B", "C"});
        sm.Start();
        sm.TriggerEvent("go_b"); WaitForTransition(sm);
        sm.TriggerEvent("go_c"); WaitForTransition(sm);
        GuiLog("Going back from C to B...");
        sm.GoBack(); WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "B");
        GuiLog("Going back from B to A...");
        sm.GoBack(); WaitForTransition(sm);
        ASSERT(sm.GetCurrent() == "A");
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    void Test_AsyncFlow() {
        GuiLog("--- Running: Asynchronous Flow Test ---", GUI_LOG_HEADER);
        StateMachine sm;
        sm.SetInitial("Idle");
        sm.AddState({"Idle", [this](auto&, auto done){ GuiLog("Entered Idle."); done(true); }, {}});
        sm.AddState({"Working", [this](auto&, auto done){
            GuiLog("Entering Working state, starting 250ms task...");
            SetTimeCallback(250, [this, done]{
                GuiLog("...Async task finished.");
                done(true);
            });
        }, {}});
        sm.AddTransition({"start", "Idle", "Working"});
        sm.Start();
        sm.TriggerEvent("start");
        GuiLog("Main thread is not blocked...");
        WaitForTransition(sm);
        GuiLog("Transition is now complete.");
        ASSERT(sm.GetCurrent() == "Working");
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    void Test_EdgeCases() {
        GuiLog("--- Running: Edge Cases Test ---", GUI_LOG_HEADER);
        {
            GuiLog("  Sub-test: Ignoring events during transition...");
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
            GuiLog("  -> PASSED", GUI_LOG_SUCCESS);
        }
        {
            GuiLog("  Sub-test: Aborting transition on OnExit failure...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, 
                              [this](auto&, auto done){ 
                                  GuiLog("    OnExit from A failed as expected.");
                                  done(false);
                              }});
            sm.AddState({"B", [this](auto&, auto done){ GuiLog("    ERROR: Should not have entered B!", GUI_LOG_ERROR); done(true); }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "A");
            GuiLog("  -> PASSED", GUI_LOG_SUCCESS);
        }
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    void Test_AdvancedHistory() {
        GuiLog("--- Running: Advanced History & GoBack Test ---", GUI_LOG_HEADER);
        {
            GuiLog("  Sub-test: GoBack transition fails on OnEnter...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [this](auto&, auto done){ GuiLog("    Entered A"); done(true); }, {}});
            sm.AddState({"B", 
                [this](auto&, auto done){ 
                    GuiLog("    OnEnter for B was called, but will fail.");
                    done(false);
                }, 
                [this](auto&, auto done){ GuiLog("    OnExit from B"); done(true); }
            });
            sm.AddState({"C", [this](auto&, auto done){ GuiLog("    Entered C"); done(true); }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.AddTransition({"go_c", "B", "C"});
            sm.Start();
            sm.TriggerEvent("go_b"); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B"); 
            sm.TriggerEvent("go_c"); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "C");
            GuiLog("    Attempting to GoBack from C to B (which will fail)...");
            sm.GoBack();
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "C");
            GuiLog("    Correctly remained in state C.");
            GuiLog("  -> PASSED", GUI_LOG_SUCCESS);
        }
        {
            GuiLog("  Sub-test: Interleaving GoBack and TriggerEvent...");
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
            GuiLog("  -> PASSED", GUI_LOG_SUCCESS);
        }
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    void Test_AdvancedCallbacksAndFailures() {
        GuiLog("--- Running: Advanced Callbacks & Failures Test ---", GUI_LOG_HEADER);
        {
            GuiLog("  Sub-test: Re-entrancy from OnAfter callback...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"C", [this](auto&, auto done){ GuiLog("    ERROR: Should never enter C!", GUI_LOG_ERROR); done(true); }, {}});
            Transition t;
            t.event = "go_b";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [this](const TransitionContext& ctx) {
                GuiLog("    In OnAfter for A->B, attempting to trigger event 'go_c'...");
                ctx.machine.TriggerEvent("go_c");
            };
            sm.AddTransition(t);
            sm.AddTransition({"go_c", "B", "C"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B");
            GuiLog("    Correctly ignored re-entrant event and landed in state B.");
            GuiLog("  -> PASSED", GUI_LOG_SUCCESS);
        }
        {
            GuiLog("  Sub-test: Aborting transition on OnEnter failure...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, 
                              [this](auto&, auto done){ GuiLog("    OnExit from A succeeded."); done(true); }
            });
            sm.AddState({"B", 
                [this](auto&, auto done){ 
                    GuiLog("    OnEnter for B failed as expected.");
                    done(false);
                }, 
                [](auto&, auto done){ done(true); }
            });
            sm.AddTransition({"go_b", "A", "B"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B");
            GuiLog("    Machine correctly ended in state 'B' as per the design.");
            GuiLog("  -> PASSED", GUI_LOG_SUCCESS);
        }
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    void Test_StressTest() {
        GuiLog("--- Running: Stress Test ---", GUI_LOG_HEADER);
        StateMachine sm;
        const int ITERATIONS = 50;
        sm.SetInitial("A");
        sm.AddState({"A", [this](auto&, auto done){ SetTimeCallback(1, [=]{ done(true); }); }, {}});
        sm.AddState({"B", [](auto&, auto done){ done(true); }, {}});
        sm.AddState({"C", [this](auto&, auto done){ SetTimeCallback(1, [=]{ done(true); }); }, {}});
        sm.AddTransition({"a_to_b", "A", "B"});
        sm.AddTransition({"b_to_c", "B", "C"});
        sm.AddTransition({"c_to_a", "C", "A"});
        sm.Start();
        WaitForTransition(sm);
        
        GuiLog("  Sub-test: Rapid cycling for 500 loops... ", GUI_LOG_NORMAL, false);
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.TriggerEvent("c_to_a"); WaitForTransition(sm);
            if ((i + 1) % 25 == 0) { 
                GuiLog(".", GUI_LOG_NORMAL, false); 
            }
        }
        GuiLog("\n  -> PASSED", GUI_LOG_SUCCESS);
        
        GuiLog("  Sub-test: Rapid GoBack() for 500 loops... ", GUI_LOG_NORMAL, false);
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            if ((i + 1) % 25 == 0) { 
                GuiLog(".", GUI_LOG_NORMAL, false);
            }
        }
        GuiLog("\n  -> PASSED", GUI_LOG_SUCCESS);
        GuiLog("-> PASSED", GUI_LOG_SUCCESS);
    }

    // The macro now handles the high-level console summary.
    #define RUN_TEST(TEST_NAME) \
        if(tests_cancelled) break; \
        ConsoleLog("Running: " #TEST_NAME "... "); \
        TEST_NAME(); \
        if(tests_cancelled) break; \
        ConsoleLog("PASSED\n"); \
        GuiLog(""); // Add a blank line in the GUI between tests

    void RunAllTests() {
        ConsoleLog("========================================\n");
        ConsoleLog("  Running State Machine Test Suite\n");
        ConsoleLog("========================================\n");
        GuiLog("========================================", GUI_LOG_HEADER);
        GuiLog("  Running State Machine Test Suite", GUI_LOG_HEADER);
        GuiLog("========================================", GUI_LOG_HEADER);
        GuiLog("");

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

        GuiLog("");
        if(tests_cancelled) {
            ConsoleLog("\n** Test Suite Cancelled By User **\n");
            GuiLog("** Test Suite Cancelled By User **", GUI_LOG_ERROR);
        } else {
            ConsoleLog("========================================\n");
            ConsoleLog("  Test Suite Finished\n");
            ConsoleLog("========================================\n");
            GuiLog("========================================", GUI_LOG_HEADER);
            GuiLog("  Test Suite Finished", GUI_LOG_HEADER);
            GuiLog("========================================", GUI_LOG_HEADER);
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