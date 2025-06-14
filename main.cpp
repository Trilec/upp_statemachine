#include "statemachine.h"

using namespace Upp;

// --- Test Harness Setup ---
// We'll use simple counters to track the results of our tests.
int tests_passed = 0;
int tests_failed = 0;

// A simple assertion macro to check conditions and report results.
// This is the core of our testing framework.
#define TEST_ASSERT(condition, description) \
    do { \
        if (condition) { \
            Cout() << "  [PASS] " << description << EOL; \
            tests_passed++; \
        } else { \
            Cout() << "  [FAIL] " << description << EOL; \
            tests_failed++; \
        } \
    } while (0)

// --- Test Cases ---

void Test_BasicTransitions() {
    Cout() << "--- Testing Basic Setup and Transitions ---" << EOL;
    StateMachine sm;
    
    bool stateA_entered = false;
    bool stateA_exited = false;
    bool stateB_entered = false;

    State stateA;
    stateA.id = "A";
    stateA.OnEnter = [&](StateMachine&, auto done) { stateA_entered = true; done(true); };
    stateA.OnExit = [&](StateMachine&, auto done) { stateA_exited = true; done(true); };
    sm.AddState(pick(stateA));

    State stateB;
    stateB.id = "B";
    stateB.OnEnter = [&](StateMachine&, auto done) { stateB_entered = true; done(true); };
    sm.AddState(pick(stateB));

    Transition t;
    t.from = "A";
    t.to = "B";
    t.event = "GO_TO_B";
    sm.AddTransition(pick(t));
    
    sm.SetInitial("A");
    sm.Start();

    TEST_ASSERT(sm.GetCurrent() == "A", "Machine should start in state A.");
    TEST_ASSERT(stateA_entered, "OnEnter for initial state A should be called on Start().");
    TEST_ASSERT(!sm.IsTransitioning(), "Machine should not be transitioning after start.");

    sm.TriggerEvent("GO_TO_B");

    TEST_ASSERT(sm.GetCurrent() == "B", "Machine should have transitioned to state B.");
    TEST_ASSERT(stateA_exited, "OnExit for state A should have been called.");
    TEST_ASSERT(stateB_entered, "OnEnter for state B should have been called.");
}

void Test_GuardsAndCallbacks() {
    Cout() << "--- Testing Guards and Callbacks ---" << EOL;
    StateMachine sm;
    
    bool allow_transition = false;
    bool before_called = false;
    bool after_called = false;

    sm.AddState({.id = "A"});
    sm.AddState({.id = "B"});

    Transition t;
    t.from = "A";
    t.to = "B";
    t.event = "TRY_GO";
    t.Guard = [&](const auto&) { return allow_transition; };
    t.OnBefore = [&](const auto&) { before_called = true; };
    t.OnAfter = [&](const auto&) { after_called = true; };
    sm.AddTransition(pick(t));
    
    sm.SetInitial("A");
    sm.Start();
    
    Cout() << "  Testing blocked transition..." << EOL;
    sm.TriggerEvent("TRY_GO");
    TEST_ASSERT(sm.GetCurrent() == "A", "Guard (false) should block transition.");
    TEST_ASSERT(before_called, "OnBefore should be called even if guard fails.");
    TEST_ASSERT(!after_called, "OnAfter should NOT be called if guard fails.");

    Cout() << "  Testing allowed transition..." << EOL;
    before_called = false; // Reset for next test
    allow_transition = true;
    sm.TriggerEvent("TRY_GO");
    TEST_ASSERT(sm.GetCurrent() == "B", "Guard (true) should allow transition.");
    TEST_ASSERT(before_called, "OnBefore should be called on allowed transition.");
    TEST_ASSERT(after_called, "OnAfter should be called on allowed transition.");
}

void Test_AsyncFlow() {
    Cout() << "--- Testing Asynchronous Flow ---" << EOL;
    StateMachine sm;
    
    sm.AddState({.id = "IDLE"});
    
    // State that simulates a successful async operation (e.g., a network request)
    State loading_success;
    loading_success.id = "LOADING_SUCCESS";
    loading_success.OnEnter = [&](StateMachine&, auto done) {
        Cout() << "  Starting successful async work..." << EOL;
        SetTimeCallback(50, [=]() {
            Cout() << "  Async work finished successfully." << EOL;
            done(true);
        });
    };
    sm.AddState(pick(loading_success));

    // State that simulates a failed async operation
    State loading_failure;
    loading_failure.id = "LOADING_FAILURE";
    loading_failure.OnEnter = [&](StateMachine&, auto done) {
        Cout() << "  Starting failing async work..." << EOL;
        SetTimeCallback(50, [=]() {
            Cout() << "  Async work failed." << EOL;
            done(false);
        });
    };
    sm.AddState(pick(loading_failure));
    
    sm.AddState({.id = "DONE"});
    sm.AddState({.id = "ERROR"});

    sm.AddTransition({"GO_SUCCESS", "IDLE", "LOADING_SUCCESS"});
    sm.AddTransition({"GO_FAILURE", "IDLE", "LOADING_FAILURE"});
    sm.AddTransition({"FINISH", "LOADING_SUCCESS", "DONE"}); // This transition happens internally
    
    sm.SetInitial("IDLE");
    sm.Start();

    // Test 1: Successful async transition
    sm.TriggerEvent("GO_SUCCESS");
    TEST_ASSERT(sm.IsTransitioning(), "Machine should be in transitioning state during async op.");
    TEST_ASSERT(sm.GetCurrent() == "IDLE", "Current state should remain 'IDLE' until async op completes.");
    
    // We need to let the U++ event loop run to process the SetTimeCallback
    Ctrl::ProcessEvents();
    Sleep(100); // Give time for the callback to fire and be processed
    
    TEST_ASSERT(!sm.IsTransitioning(), "Machine should NOT be transitioning after async op completes.");
    TEST_ASSERT(sm.GetCurrent() == "LOADING_SUCCESS", "Machine should be in 'LOADING_SUCCESS' state after success.");

    // Test 2: Failed async transition
    sm.SetInitial("IDLE"); // Reset for next test
    sm.Start();
    sm.TriggerEvent("GO_FAILURE");
    TEST_ASSERT(sm.IsTransitioning(), "Machine should be in transitioning state during failing async op.");

    Ctrl::ProcessEvents();
    Sleep(100);

    TEST_ASSERT(!sm.IsTransitioning(), "Machine should NOT be transitioning after async op fails.");
    TEST_ASSERT(sm.GetCurrent() == "IDLE", "Machine should revert to 'IDLE' state after failure.");
}

void Test_HistoryAndGoBack() {
    Cout() << "--- Testing History and GoBack() ---" << EOL;
    StateMachine sm;
    sm.AddState({.id = "A"});
    sm.AddState({.id = "B"});
    sm.AddState({.id = "C"});
    sm.AddTransition({"A_TO_B", "A", "B"});
    sm.AddTransition({"B_TO_C", "B", "C"});
    
    sm.SetInitial("A");
    sm.Start();
    
    TEST_ASSERT(!sm.CanGoBack(), "Should not be able to go back from initial state.");
    
    sm.TriggerEvent("A_TO_B");
    TEST_ASSERT(sm.GetCurrent() == "B", "State should be B.");
    TEST_ASSERT(sm.CanGoBack(), "Should be able to go back from state B.");

    sm.TriggerEvent("B_TO_C");
    TEST_ASSERT(sm.GetCurrent() == "C", "State should be C.");
    TEST_ASSERT(sm.CanGoBack(), "Should be able to go back from state C.");
    
    sm.GoBack();
    TEST_ASSERT(sm.GetCurrent() == "B", "GoBack() should return to state B.");
    TEST_ASSERT(sm.CanGoBack(), "Should still be able to go back from state B.");

    sm.GoBack();
    TEST_ASSERT(sm.GetCurrent() == "A", "GoBack() should return to state A.");
    TEST_ASSERT(!sm.CanGoBack(), "Should not be able to go back from initial state again.");
    
    sm.GoBack(); // Try to go back when not possible
    TEST_ASSERT(sm.GetCurrent() == "A", "GoBack() when not possible should do nothing.");
}

void Test_EdgeCases() {
    Cout() << "--- Testing Edge Cases and Error Handling ---" << EOL;
    StateMachine sm;
    sm.AddState({.id = "A"});
    sm.AddState({.id = "B"});
    sm.AddTransition({"A_TO_B", "A", "B"});
    sm.SetInitial("A");
    sm.Start();
    
    sm.TriggerEvent("INVALID_EVENT");
    TEST_ASSERT(sm.GetCurrent() == "A", "Triggering an invalid event should not change state.");
    
    // Test triggering event while busy
    State busyState;
    busyState.id = "BUSY";
    busyState.OnEnter = [&](StateMachine& m, auto done) {
        // While in this OnEnter, the machine is "transitioning"
        // Try to trigger another event from inside the callback
        m.TriggerEvent("A_TO_B"); 
        SetTimeCallback(50, [=](){ done(true); }); // Finish the transition
    };
    sm.AddState(pick(busyState));
    sm.AddTransition({"GO_BUSY", "A", "BUSY"});
    
    sm.TriggerEvent("GO_BUSY");
    Ctrl::ProcessEvents();
    Sleep(100);
    
    TEST_ASSERT(sm.GetCurrent() == "BUSY", "Nested event trigger during transition should be ignored.");
}

void Test_StressTest() {
    Cout() << "--- Performing Stress Test ---" << EOL;
    StateMachine sm;
    sm.AddState({.id = "PING"});
    sm.AddState({.id = "PONG"});
    sm.AddTransition({"DO_PONG", "PING", "PONG"});
    sm.AddTransition({"DO_PING", "PONG", "PING"});
    sm.SetInitial("PING");
    sm.Start();
    
    const int iterations = 50000;
    Cout() << "  Triggering " << iterations * 2 << " transitions..." << EOL;
    
    TimeStop ts;
    for (int i = 0; i < iterations; ++i) {
        sm.TriggerEvent("DO_PONG");
        sm.TriggerEvent("DO_PING");
    }
    
    long elapsed_ms = ts.Elapsed();
    Cout() << "  Completed in " << elapsed_ms << " ms." << EOL;
    
    TEST_ASSERT(sm.GetCurrent() == "PING", "Final state should be PING after even number of transitions.");
    TEST_ASSERT(elapsed_ms < 2000, "Performance should be reasonable (e.g., < 2s for 100k transitions).");
}


// --- Main Test Runner ---
GUI_APP_MAIN
{
    Cout() << "========================================" << EOL;
    Cout() << "  Running State Machine Test Suite" << EOL;
    Cout() << "========================================" << EOL << EOL;

    Test_BasicTransitions();
    Cout() << EOL;
    Test_GuardsAndCallbacks();
    Cout() << EOL;
    Test_HistoryAndGoBack();
    Cout() << EOL;
    Test_EdgeCases();
    Cout() << EOL;
    Test_StressTest();
    Cout() << EOL;
    
    // Async test needs the main loop to run, so we do it last before the summary.
    Test_AsyncFlow();
    
    // Post a final callback to print the summary after all async tests are done.
    SetTimeCallback(250, [] {
        Cout() << "========================================" << EOL;
        Cout() << "  Test Suite Finished" << EOL;
        Cout() << "========================================" << EOL;
        Cout() << "  PASSED: " << tests_passed << EOL;
        Cout() << "  FAILED: " << tests_failed << EOL;
        Cout() << "========================================" << EOL;
        
        // Exit the application
        if(tests_failed == 0)
            SetExitCode(0);
        else
            SetExitCode(1);
            
        BreakLoop();
    });
    
    Ctrl::EventLoop();
}