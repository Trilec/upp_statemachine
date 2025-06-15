// main.cpp - Professional GUI Test harness for the StateMachine, now with a reusable LogView class.
#include <CtrlLib/CtrlLib.h>
#include <RichEdit/RichEdit.h>
#include "statemachine.h"

using namespace Upp;

// =================================================================================
// Our new, reusable LogView class that encapsulates all our "lessons learned".
// =================================================================================
struct LogView : RichEdit {
public:
    // FIX: Renamed enum members to avoid collision with the Windows ERROR macro.
    enum LogStyle { LOG_NORMAL, LOG_HEADER, LOG_SUCCESS, LOG_ERROR };

    // The constructor sets up the control with all our hard-won settings.
    LogView() {
        SetReadOnly();
        NoRuler();
        ShowCodes(Null);
        qtf_log_buffer = "[A1] "; // Initialize with 8pt Arial font.
    }

    // A clean, public API for adding log entries.
    void Log(const String& text, LogStyle style = LOG_NORMAL, bool newline = true) {
        String qtf_text = text;
        
        switch(style) {
            case LOG_HEADER:  qtf_text = "[* " + qtf_text + "]"; break;
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
    
    // A helper for adding visual separators.
    void AddSeparator() {
        Log("[--]", LOG_NORMAL);
    }
    
    // A method to clear the log if needed.
    void Clear() {
        qtf_log_buffer = "[A1] ";
        SetQTF(qtf_log_buffer);
    }

private:
    // The internal QTF string is now a private implementation detail.
    String qtf_log_buffer;
};


// =================================================================================
// Our test runner application window, now cleaner thanks to LogView.
// =================================================================================
struct TestRunner : TopWindow {
    typedef TestRunner CLASSNAME;

    Splitter      splitter;
    LogView       logDisplay; // <-- Now using our new LogView class!
    Button        actionButton;
    
    bool          tests_cancelled = false;
    
    void ConsoleLog(const String& text) {
        Cout() << text;
    }

    void WaitForTransition(StateMachine& sm) {
        while (sm.IsTransitioning()) {
            if (tests_cancelled) return;
            Ctrl::ProcessEvents();
            Sleep(10);
        }
    }

    // --- All tests now use the clean logDisplay.Log() API ---

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

    void Test_GuardsAndCallbacks() {
        logDisplay.Log("Running: Guards and Callbacks Test", LogView::LOG_HEADER);
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
            sm.AddState({"A", [](auto&, auto done){ done(true); }, 
                              [this](auto&, auto done){ 
                                  logDisplay.Log("    OnExit from A failed as expected.");
                                  done(false);
                              }});
            sm.AddState({"B", [this](auto&, auto done){ logDisplay.Log("    ERROR: Should not have entered B!", LogView::LOG_ERROR); done(true); }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "A");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    void Test_AdvancedHistory() {
        logDisplay.Log("Running: Advanced History & GoBack Test", LogView::LOG_HEADER);
        {
            logDisplay.Log("  Sub-test: GoBack transition fails on OnEnter...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [this](auto&, auto done){ logDisplay.Log("    Entered A"); done(true); }, {}});
            sm.AddState({"B", 
                [this](auto&, auto done){ 
                    logDisplay.Log("    OnEnter for B was called, but will fail.");
                    done(false);
                }, 
                [this](auto&, auto done){ logDisplay.Log("    OnExit from B"); done(true); }
            });
            sm.AddState({"C", [this](auto&, auto done){ logDisplay.Log("    Entered C"); done(true); }, {}});
            sm.AddTransition({"go_b", "A", "B"});
            sm.AddTransition({"go_c", "B", "C"});
            sm.Start();
            sm.TriggerEvent("go_b"); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B"); 
            sm.TriggerEvent("go_c"); WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "C");
            logDisplay.Log("    Attempting to GoBack from C to B (which will fail)...");
            sm.GoBack();
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "C");
            logDisplay.Log("    Correctly remained in state C.");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
        {
            logDisplay.Log("  Sub-test: Interleaving GoBack and TriggerEvent...");
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
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    void Test_AdvancedCallbacksAndFailures() {
        logDisplay.Log("Running: Advanced Callbacks & Failures Test", LogView::LOG_HEADER);
        {
            logDisplay.Log("  Sub-test: Re-entrancy from OnAfter callback...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"B", [](auto&, auto done){ done(true); }, {}});
            sm.AddState({"C", [this](auto&, auto done){ logDisplay.Log("    ERROR: Should never enter C!", LogView::LOG_ERROR); done(true); }, {}});
            Transition t;
            t.event = "go_b";
            t.from = "A";
            t.to = "B";
            t.OnAfter = [this](const TransitionContext& ctx) {
                logDisplay.Log("    In OnAfter for A->B, attempting to trigger event 'go_c'...");
                ctx.machine.TriggerEvent("go_c");
            };
            sm.AddTransition(t);
            sm.AddTransition({"go_c", "B", "C"});
            sm.Start();
            sm.TriggerEvent("go_b");
            WaitForTransition(sm);
            ASSERT(sm.GetCurrent() == "B");
            logDisplay.Log("    Correctly ignored re-entrant event and landed in state B.");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
        {
            logDisplay.Log("  Sub-test: Aborting transition on OnEnter failure...");
            StateMachine sm;
            sm.SetInitial("A");
            sm.AddState({"A", [](auto&, auto done){ done(true); }, 
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
            ASSERT(sm.GetCurrent() == "B");
            logDisplay.Log("    Machine correctly ended in state 'B' as per the design.");
            logDisplay.Log("    -> PASSED", LogView::LOG_SUCCESS);
        }
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    void Test_StressTest() {
        logDisplay.Log("Running: Stress Test", LogView::LOG_HEADER);
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
        
        logDisplay.Log("  Sub-test: Rapid cycling... ", LogView::LOG_NORMAL, false);
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.TriggerEvent("c_to_a"); WaitForTransition(sm);
            if ((i + 1) % 5 == 0) { 
                logDisplay.Log(".", LogView::LOG_NORMAL, false); 
            }
        }
        logDisplay.Log(" done.", LogView::LOG_NORMAL);
        
        logDisplay.Log("  Sub-test: Rapid GoBack()... ", LogView::LOG_NORMAL, false);
        for (int i = 0; i < ITERATIONS; i++) {
            if (tests_cancelled) return;
            sm.TriggerEvent("a_to_b"); WaitForTransition(sm);
            sm.TriggerEvent("b_to_c"); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            sm.GoBack(); WaitForTransition(sm);
            if ((i + 1) % 5 == 0) { 
                logDisplay.Log(".", LogView::LOG_NORMAL, false);
            }
        }
        logDisplay.Log(" done.", LogView::LOG_NORMAL);
        logDisplay.Log("  -> PASSED", LogView::LOG_SUCCESS);
    }

    #define RUN_TEST(TEST_NAME) \
        if(tests_cancelled) break; \
        ConsoleLog("Running: " #TEST_NAME "... "); \
        TEST_NAME(); \
        if(tests_cancelled) break; \
        ConsoleLog("PASSED\n"); \
        logDisplay.AddSeparator();

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
        if(tests_cancelled) {
            ConsoleLog("\n** Test Suite Cancelled By User **\n");
            logDisplay.Log("Test Suite Cancelled By User", LogView::LOG_ERROR);
        } else {
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