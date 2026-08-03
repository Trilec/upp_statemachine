# Gary's notes — SM-002-R1 callback-lifetime failure

## Resolution (SM-002-R1A)

The debug-heap corruption was caused by an invalid test, not by the retained
completion observer. `Settlement callback may destroy its owner` originally
installed `WhenTransitionSettled` before synchronous `Start()`. Startup
settlement destroyed the `StateMachine`; the test then called `raw->TriggerEvent`
through that dangling pointer. The debug heap detected the resulting prior write
on allocation of the following test's machine.

The test now starts synchronously first, installs a settlement callback that
destroys the owner only for event `go`, triggers `go`, and never touches `raw`
afterward. The normal U++ debug-heap core run completes cleanly.

The isolated proof in `StateMachineCoreTest` also passes: `One<Pte<T>>`,
`Ptr<T>`, copied/picked `Function<void(bool)>`, and the by-value completion
argument shape all safely ignore callbacks after the observed object is cleared.

Final observer shape:

```text
StateMachine owns One<Operation>
Operation derives Pte<Operation>
Completion functor contains Ptr<Operation> + callback phase
```

`Operation` is released during cancellation, settlement, and destruction, so a
retained completion observes null and does nothing. A separate `One<Lifetime>` /
`Ptr<Lifetime>` guards code after user hooks. The destructor first nulls the
lifetime owner, then clears the active operation and lifetime record. User hooks
are copied to local `Function` values before invocation; ordinary U++
`Function::operator()` does not self-retain, so caller-owned callback storage and
captures remain caller-owned.

Validated after the correction: debug-heap core suite `218/218`,
`StateMachineGuiTest` build, and `StateMachineVisualizer` build plus launch.

## Visualizer and UI follow-up

The Visualizer now includes a non-core demonstration of the lifecycle work:

- an automatic periodic lifecycle check with an enable toggle and interval slider;
- an `Async Monitor` diagnostic/inspection node with dotted routes;
- sampled Quality Check units enter that node, then asynchronously route either
  to Packaging or to Quality Review; and
- the Manufacturing Log records cancellation, stale-completion rejection, and
  monitor approval/escalation outcomes.

This is example-only code; the reusable StateMachine package remains Core-only
and contains no UI, timer, worker, graph, or workflow dependency.

While testing node interaction, an independent `upp_Ui` bug was found in
`UiTitleCard::CancelMode`. It called `ReleaseCapture`, while U++ itself calls
`CancelMode` from `ReleaseCapture`, causing recursive/nested release and an
invalid capture-control access. The fix makes `CancelMode` state-only: it clears
the pressed/capture flags and leaves capture release to the caller/framework.
The Visualizer rebuilt after that fix and node clicking no longer recurses.

## Remaining validation note

The normal U++ debug-heap core run is the authoritative completed validation.
An AddressSanitizer CLANGx64 run was not performed in this local pass because no
confirmed U++ CLANGx64 ASan build configuration/link flags were available; it
remains a recommended follow-up rather than a substituted result.

## SM-002-R2 continuation

R2 begins at commit `8eb4be821cfbea385ec7a52e683430d4fed1c573` and retains the
accepted completion observer (`One<Operation>`, `Pte<Operation>`,
`Ptr<Operation>`). No shared ownership, custom reference counting, registry,
thread, lock, or scheduler was introduced.

Implemented and validated in the reusable core:

- `TriggerEvent()` and `TryTransition()` copy transition/guard data before
  invocation, acquire a `Ptr<Lifetime>` observer, and use a monotonic guard
  preflight generation plus source-state check. A guard that resets, clears,
  destroys the owner, or causes a nested guard/transition cannot leave the outer
  dispatch continuing against stale state.
- Guard evaluation remains before accepted-operation creation, so it still
  observes `IsTransitioning()==false`; rejected guards do not publish settlement.
- `DrainQueuedEvents()` retains a lifetime observer across every nested
  `TriggerEvent()`. Destruction during a queued callback returns immediately and
  does not touch queue state or dispatch a second event.
- Added deterministic tests for guard reset invalidation, guard destruction for
  both `TriggerEvent` and `TryTransition`, and owner destruction during queued
  dispatch. Debug-heap Core result is now **221/221 passed**.

Visualizer R2 work changes the previous repetitive self-test / fixed-route demo
to a persistent example-owned audit StateMachine (`DORMANT` and `AUDITING`). It
arms at an interval or through `Run spot check now`, samples one eligible Quality
Check unit, performs asynchronous inspection, then uses the configured ordinary
pass/review policy to route it. Cancellation records a late-result-ignored event.
This remains example-only; no timer or GUI concept enters `statemachine`.

Current handoff status: Core debug-heap and StateMachineGuiTest builds are green.
The final Visualizer smoke test, remaining result-matrix additions, documentation
consolidation, diff/status checks, and local R2 commit still need completion.

## Original review request and investigation record

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

## SM-002-R3 completion note

R3 retains `One<Operation>` / `Pte<Operation>` / `Ptr<Operation>` completion
ownership. Guard dispatch now validates a runtime/configuration generation
before handling either result. The debug-heap Core suite passes 224/224 tests.
The spot-check visualizer retains unit, sequence, forced-review decision,
completion, and route-once state; cancellation routes the item once and a late
completion is visibly ignored. Shipment tokens retain their batch quantity.
