# Gary's notes — SM-002-R1 callback-lifetime failure

## Review requested

Please review the asynchronous completion-callback ownership design in
`statemachine/statemachine.h` and `statemachine/statemachine.cpp`, with special
attention to U++ `Function`, `Pte`, `Ptr`, `One`, and completion callbacks that
outlive a `StateMachine` instance.

The goal remains SM-002/R1: a compact Core-only FSM with explicit cancellation,
exact-once structured settlement, stale-completion rejection, and harmless
retained `done(bool)` callbacks after cancellation or destruction. It must stay
same-thread/same-callback-chain only; no mutexes, threads, timers, schedulers,
or workflow concepts have been introduced.

## Current branch and baseline

- Branch: `feature/async-lifecycle-hardening`
- Last committed SHA: `5d649349e9ef49aecdd2b10c736c281ec11bfd38`
- This handoff is intentionally uncommitted and incomplete.
- Original expected baseline: core suite `190/190`, GUI build, Visualizer build
  and launch.

## Reproducible failure

The debug core executable builds, and all tests reported before the new retained
callback tests pass. The process then hits U++ debug heap detection:

```text
Writes to freed blocks detected
Upp::Panic(...)
Upp::Heap::DbgFreeCheckK(...)
Upp::Heap::Allok(...)
operator new(size=320)
...
tests/StateMachineCoreTest/main.cpp:4990
```

Line 4990 is allocation of the next `StateMachine` in the test named
`Retained exit, enter, and back callbacks survive destruction`. The heap checker
therefore detects a previous write; it does not prove the allocation on line 4990
is at fault.

The immediately preceding focused test is:

```cpp
Function<void(bool)> done;
{
    StateMachine sm;
    sm.SetInitial("A");
    sm.AddState({"A", [&](auto&, auto d) { done = d; }, {}});
    sm.Start();
}
done(true);
done(false);
```

It prints `PASSED` before the heap checker trips. The external `done` is meant
to be harmless after `sm` has been destroyed. The same failure was observed when
the test only cleared the retained function, so the issue may be destruction or
retention rather than callback execution alone.

## Current attempted design

The current uncommitted implementation is an in-progress native-U++ redesign:

- `StateMachine` owns one active `Operation` through `One<Operation>`.
- `Operation` derives from `Pte<Operation>` and records sequence, operation
  kind, outcome, phase (`Active`, `Claiming`, `Settled`), awaited callback phase,
  and the owning machine pointer.
- The concrete `Completion` functor holds `Ptr<Operation>` and a callback phase.
  It contains no lambda capture, shared pointer, weak pointer, or raw machine
  pointer. If the machine deletes the active operation, the intent is that U++
  nulls the retained `Ptr` before a caller can invoke it.
- `Lifetime : Pte<Lifetime>` is currently retained only around user hooks and
  settlement callbacks so code can detect a machine destroyed re-entrantly while
  a callback was executing.
- `SettleClaimed` is the central terminal path. It publishes the stable result,
  clears active/transitioning state, calls `WhenTransitionSettled`, then drains
  the queue only after successful settlement and only if the lifetime marker says
  the machine still exists.

This code compiles, but the debug-heap failure above remains. It must not be
merged or committed as a finished implementation in this state.

## Earlier approaches tried and rejected

1. Lambdas capturing `std::shared_ptr<Operation>` and a lifetime record.
   - Rejected because the failure remained and this mixed C++ ownership objects
     directly into U++ `Function` wrappers.

2. A custom ref-counted completion token containing U++ `Ptr<Lifetime>` plus
   `std::shared_ptr<Operation>`.
   - Rejected because retained callback tests still caused debug-heap failure.

3. A concrete completion functor storing `std::weak_ptr<Operation>`.
   - Rejected because the same failure remained.

4. Current U++ `Pte<Operation>` / `Ptr<Operation>` observer approach.
   - This is the smallest and most U++-idiomatic approach researched so far, but
     it has not fixed the heap failure.

No test has been disabled or skipped to hide the issue.

## Relevant U++ observations

`E:\upp-18468\uppsrc\Core\Function.h` shows `Function` reference-counts its
wrapper, but ordinary `Function::operator()` does not take an additional
self-reference while executing. `Wrapper2` does explicitly self-retain. This
may matter for re-entrant destruction and for a completion callback which can
cause its own stored function to be released.

The U++ UI guidance recommends `Pte`/`Ptr` for observed lifetime. The current
operation-based use follows that guidance, but needs review for whether a
`Ptr<Operation>` is safe when the `Operation` is owned by `One<Operation>` and
stored inside a `Function` which is copied/moved through callback parameters.

## Behaviour already implemented and covered before the heap stop

- Structured `TransitionResult`, outcomes, operation kind, and monotonic sequence.
- `CancelActiveTransition`, active/last sequence queries, `NoActiveTransition`.
- Success-only `WhenTransitionFinished` and transition `OnAfter`.
- Structured `WhenTransitionSettled` for success, failure, and cancellation.
- Startup, normal transition, and GoBack cancellation/rollback semantics.
- Cancellation preserves normal-transition current state/history/queue; startup
  cancellation clears startup runtime/history/queue.
- Queue drain is stopped after a synchronous cancellation.
- Re-entrant cancellation from started/before/enter/exit hooks and settlement
  callback destruction cases are present in the core suite.

These behaviours require a clean rerun after the lifetime defect is fixed.

## Questions for review

1. What is the correct U++ ownership pattern for a caller-retained
   `Function<void(bool)>` supplied by a class that may be destroyed first?
2. Is `Ptr<Operation>` inside a functor stored by `Function` valid with
   `One<Operation>` ownership, including the `Function` argument copies made by
   the `State::OnEnter`/`OnExit` signatures?
3. Does `Function::operator()` need an explicit self-retaining proxy or another
   invocation mechanism for this use case?
4. Should the completion authority be a separately allocated `Pte` record rather
   than the active operation itself, and if so what ownership shape avoids both
   raw-machine dereference and retained-machine leaks?
5. Please identify any post-callback member access or incorrect `One`/`Ptr`
   clearing order in the current code that could write into freed storage.

## Validation still required after resolution

1. Run `StateMachineCoreTest` to normal completion under debug heap checking.
2. Verify the full original suite plus all SM-002/R1 deterministic cases.
3. Build `examples/StateMachineGuiTest`.
4. Build and smoke-launch `examples/StateMachineVisualizer` with the supplied
   `upp_animation` and `upp_Ui` source locations.
5. Review all changed implementation/tests/docs and update README, API, DESIGN,
   CHANGELOG, and the handover notes accurately.

