# AgentFlow State Machine Handover

## 1. Project Identity and Purpose

- **Project:** compact asynchronous finite-state machine for Ultimate++.
- **Repository:** `Trilec/upp_statemachine`.
- **Default branch:** `main`.
- **Published HEAD verified on 2026-08-02:** `93e20f109181e16667176214484cb4f585a3d2b8`.
- **Reported local path:** `E:\apps\github\upp_statemachine`.
- **Purpose:** provide a deterministic, reusable, GUI-independent lifecycle engine with asynchronous enter/exit operations.
- **Current phase:** SM-002 is implemented locally; SM-002-R1 closes re-entrant settlement and lifetime gaps pending supervisor review.
- **Boundary:** this remains a one-current-state Core-only FSM. It does not become a graph scheduler, agent runtime, GUI system, tool broker, conversation layer, or MCP component.

## 2. Roles, Workflow, and Task Standards

- **Curt**: Project manager, product owner, decision-maker, final approver, and publisher. Curt normally performs pushes, tags, releases, and publication. Depending on the project workflow, Curt may also perform commits.
- **Implementer**: Primary implementation programmer and may also be assigned small, tightly bounded assemblies. Edits code, runs local builds and tests, performs diagnostics, creates permitted local commits, and reports results.
  - **Critical Git rules**: Due to permission restrictions, the implementer may commit locally but must **never push**. Every requested commit must have a concise title or summary of approximately 90 characters or fewer and a meaningful multi-line description explaining the implementation and validation.
  - **Assignment language**: Use contributor-neutral, direct imperative instructions. Do not name the implementer in task assignments.
- **ChatGPT Supervisor**: Inspects the repository, reviews implementation and architecture, accepts or rejects work, tracks progress against the plan, prepares precise next tasks, and protects the project’s technical direction. Apply the same review standards regardless of who performed the implementation.

### Task Sizing and Grouping Standard

Divide work into focused tasks that are substantial enough to advance the project but narrow enough that the objective, affected code paths, completion boundary, and validation are unambiguous.

Every task must have:

1. **One primary outcome.** Multiple edits are acceptable only when they directly support one central technical result.
2. **A coherent affected area.** Group changes only within the same subsystem, lifecycle, data flow, interaction, defect family, or validation path.
3. **A coherent validation loop.** Work requiring substantially different validation paths should normally be separated.
4. **An explicit completion boundary.** Completion means demonstrable behaviour, not merely changed files.
5. **Controlled file and concept spread.** Cross-layer work is allowed only when required for one vertical slice.
6. **Stability before expansion.** Correct crashes, corruption, lifecycle faults, regressions, failing tests, and invalid architecture before adding features.
7. **No opportunistic scope growth.** Preserve nearby improvements as deferred follow-ups unless required for correctness.
8. **Small-agent viability.** The assignment must not require reconstructing the entire project or making undeclared architecture decisions.
9. **No artificial fragmentation.** Keep inseparable corrections together when splitting would create invalid intermediate states, duplicated investigation, or meaningless validation.

Practical test:

> Can the objective, reason, affected subsystem, exclusions, completion condition, and validation strategy each be explained clearly without introducing a second independent objective?

If not, divide the task.

## 3. Build and Repository Environment

| Item | State |
|---|---|
| U++ root | **Reported:** `E:\upp-18468` |
| `umk.exe` | **Reported:** `E:\upp-18468\umk.exe` |
| Toolchain | **Reported:** `CLANGx64` |
| Assembly | **GitHub verified:** `GitHubOut.var` |
| Assembly nests | **GitHub verified:** `E:/apps/github/upp_statemachine;E:/upp-18468/uppsrc` |
| Output | **GitHub verified:** `E:/apps/github/upp_statemachine/build` |
| Core dependency | **GitHub verified:** reusable package depends only on `Core` |
| Exact CLI commands | **Not documented authoritatively:** use TheIDE/`umk` with `GitHubOut.var` and report exact commands |

Published packages:

```text
statemachine/statemachine.upp
tests/StateMachineCoreTest/StateMachineCoreTest.upp
examples/StateMachineGuiTest/StateMachineGuiTest.upp
examples/StateMachineVisualizer/StateMachineVisualizer.upp
```

The GUI examples are optional and do not change the core dependency boundary.

## 4. Architecture and Supervisory Standards

Accepted behaviour:

- one configured set of flat states and transitions;
- one current state;
- asynchronous `OnExit` and `OnEnter` callbacks with completion;
- guards and transition hooks;
- history and `GoBack()`;
- reset/reuse and clear;
- bounded event-name queue while transitioning;
- same-thread/same-callback-chain use;
- no internal locking or worker threads.

Required hardening:

- monotonic active-transition sequence identity;
- explicit cancellation of the active operation;
- exact-once terminal settlement;
- stale and duplicate completion rejection;
- lifetime-safe callbacks after machine destruction;
- structured terminal result while preserving legacy `done(bool)` behaviour;
- precise startup, normal transition, and `GoBack()` cancellation semantics;
- deterministic queue behaviour after cancellation.

Ownership boundaries:

- caller/runtime owns timeouts and decides when to request cancellation;
- FSM owns its transition lifecycle and refuses stale completions;
- cancellation does not reverse side effects already performed by user callbacks;
- AgentFlowRuntime later owns node scheduling, joins, retries, provider/tool calls, and graph execution.

Strict prohibitions:

- no graph nodes, edges, parallel regions, or workflow scheduler;
- no agent, provider, evidence, or tool types;
- no GUI or `CtrlLib` dependency in `statemachine`;
- no mutexes, threads, timers, sleeps, or hidden dispatch;
- no MCP;
- no silent semantic change to queue/history/guard/hook contracts;
- no unsafe callback that dereferences a destroyed machine;
- no success report based only on compilation.

## 5. Compressed History and Component Status

### Accepted milestones

- v1.0.1 provides validated state/transition configuration, asynchronous enter/exit, guards/hooks, history, reset/reuse, error reporting, and bounded event queueing.
- The authoritative Core test baseline is 190/190; GUI test builds; visualizer builds and launches.
- Queue draining has a bounded synchronous cycle limit and preserves remaining queued events on refusal.
- AgentFlow will use this FSM only for high-level run phases, not for individual graph nodes.

### Rejected or superseded approaches

- **Turn the FSM into the AgentFlow scheduler: rejected.** Graph scheduling belongs in `AgentFlowRuntime`.
- **Add internal timers/threads for timeout handling: rejected.** Timeout policy remains caller-owned.
- **Require the machine to outlive unsafe callbacks forever: superseded.** StateMachine-supplied callbacks use a lifetime token; arbitrary caller captures remain caller-owned.
- **Use cancellation as rollback of user side effects: rejected.** Cancellation settles ownership only.
- **Add MCP or GUI hooks to core: rejected.** Those belong above the package.

### Component status

| Component | Purpose | Current status | Limits |
|---|---|---|---|
| `statemachine` | Reusable FSM | Published v1.0.1 | No cancellation/lifetime-safe late callback yet |
| `StateMachineCoreTest` | Authoritative regression suite | Published 190/190 | Needs async cancellation/lifetime matrix |
| `StateMachineGuiTest` | GUI build/manual check | Published | Not behavioural authority for core |
| `StateMachineVisualizer` | Visual/manual harness | Published | Reference only; must not drive core design |
| Structured settlement | Terminal async outcome | Implemented; R1 validation pending | Supervisor review |
| Cancellation | Active transition cancellation | Implemented; R1 validation pending | Supervisor review |

## 6. Current Verified State and Active Milestone

### GitHub verified

- `main` HEAD is `93e20f109181e16667176214484cb4f585a3d2b8`.
- README declares v1.0.1 and the 190/190 Core baseline.
- `statemachine` depends only on `Core`.
- Current API documents structured cancellation and lifetime-safe StateMachine-supplied callbacks.

### Locally reported

No async-hardening implementation report or local commit has been supplied. Local worktree state is unknown.

### Active milestone

**`SM-002 — Harden asynchronous lifecycle cancellation and settlement`**

Objective:

> Add cancellation, exact-once structured settlement, stale completion protection, and lifetime-safe asynchronous callbacks without redesigning the FSM or changing its same-thread Core-only contract.

Completed: v1.0.1 baseline; architecture and exact task specification; required queue/result semantics identified.

Remaining: preflight; API/internal design; implementation for startup/transition/GoBack; deterministic test matrix; docs/changelog; local commit/report; supervisor review.

Acceptance criteria:

- all existing behaviour remains green;
- cancellation settles exactly once;
- stale/duplicate callbacks do not mutate state or drain queues;
- callback after destruction is safe;
- startup, normal transition, and GoBack paths have explicit tested results;
- queue behaviour matches the task specification;
- no new dependency, thread-safety claim, timer, graph, GUI, agent, or MCP feature.

Blockers: none published; local worktree and exact build commands need preflight.

Estimated completion:

- **Published v1.0.1 baseline: 100%.**
- **SM-002 specification: approximately 95%.**
- **SM-002 implementation: implemented locally.**

Continue from: verify branch/HEAD/worktree; run 190/190 Core and GUI/example baseline; inspect all async paths; execute the exact task below.

## 7. Validation Baseline

| Validation | Accepted baseline / requirement |
|---|---|
| `StateMachineCoreTest` | Published baseline 190/190 |
| New cancellation tests | Pending exit, pending enter, startup, GoBack, duplicate/stale callback, repeat cancellation |
| Lifetime tests | Completion after machine destruction must be harmless |
| Queue tests | Retain/no auto-drain on normal cancellation; clear startup queue; stale callback cannot drain |
| Reset/Clear tests | Reject while active; work after cancellation settles |
| `StateMachineGuiTest` | Must build |
| `StateMachineVisualizer` | Must build and launch |
| Determinism | No sleeps, timers, threads, or wall-clock dependence |
| Hygiene | `git diff --check`; clean intended worktree after local commit |

Exact command lines must be reported because no single authoritative CLI sequence is published.

## 8. Future Plan and Exact Next Task

### Master plan

```text
SM-002 accepted
UI-NODEGRAPH-001 accepted
    ↓
AgentFlowCore and runtime may begin
```

Authoritative task source: `Trilec/upp_agentflow/docs/AgentFlow_StateMachine_Adjustments.md`.

### Corrective task

**Task ID:** `SM-002-R1`  
**Title:** Harden asynchronous lifecycle cancellation and settlement

**Current context:** asynchronous completion currently requires owner lifetime and has no cancellation.

**Primary objective:** Make every asynchronous operation safely cancelable and exactly-once settled.

**Why next:** real model/tool phases can outlive runs or owners; AgentFlow cannot safely depend on the FSM until stale and late callbacks are harmless.

**In scope:** inspect startup, transition, enter, exit, GoBack, queue, reset, and clear paths; add monotonic operation identity; add structured terminal result and settlement callback while preserving compatibility; add `CancelActiveTransition()` or accepted equivalent; add lifetime-safe weak ownership; reject stale/duplicate completion; define normal/startup/GoBack cancellation state/history outcomes; preserve caller-owned timeout; implement queue semantics; update docs; add deterministic tests.

**Likely areas:**

```text
statemachine/statemachine.h
statemachine/statemachine.cpp
tests/StateMachineCoreTest/main.cpp
examples/StateMachineGuiTest/main.cpp
docs/API.md
docs/DESIGN.md
README.md
CHANGELOG.md
```

**Constraints:** Core-only; same-thread; preserve current configuration APIs and established guard/hook/history/event-policy behaviour; exact-once settlement; stale-safe after cancellation/destruction; deterministic tests.

**Explicit exclusions:** graph scheduling; parallel/hierarchical states; timers; threads/locking; retry policy; AgentFlow types; UI integration; MCP.

**Completion boundary:** one local commit with the full cancellation/lifetime/result contract, complete tests, documentation, and required build results.

**Required local commit title:**

```text
StateMachine hardens async cancellation and stale completion
```

**Required commit description:**

```text
Add sequence-owned asynchronous operations, explicit cancellation, structured
terminal outcomes, and exact-once settlement for startup, transitions, and
GoBack.

Reject stale and duplicate completions, make late callbacks lifetime-safe, keep
timeout policy caller-owned, preserve queue/history contracts, and extend the
deterministic Core regression matrix.
```

**Implementation report:** starting/resulting commit; files changed; API and lifetime design; cancellation semantics; queue/history/reset/clear behaviour; commands/totals; GUI/example checks; `git status`; `git diff --check`; confirm local commit only and no push.

### Deferred follow-ups

- AgentFlow lifecycle integration after acceptance;
- version/release decision;
- unrelated hierarchical or parallel-state features.

## 9. Essential Reading and Open Questions

### Essential reading

1. `README.md` — published features, baseline, limits.
2. `statemachine/statemachine.h` — public API and callback types.
3. `statemachine/statemachine.cpp` — lifecycle implementation.
4. `statemachine/statemachine.upp` — Core-only dependency authority.
5. `tests/StateMachineCoreTest/main.cpp` — authoritative regression suite.
6. `examples/StateMachineGuiTest/main.cpp` — GUI usage/build check.
7. `examples/StateMachineVisualizer/VisualizerApp.h/.cpp` — async/manual reference.
8. `examples/StateMachineVisualizer/VisualizerModel.h` — visualizer use.
9. `docs/API.md` — behavioural contract.
10. `docs/DESIGN.md` — ownership/lifecycle boundaries.
11. `CHANGELOG.md` — published release state.
12. `GitHubOut.var` — local assembly/output.
13. `Trilec/upp_agentflow/docs/AgentFlow_StateMachine_Adjustments.md` — active task.
14. `Trilec/upp_agentflow/docs/AgentFlow_Architecture.md` — future composition boundary.

### Open questions

**Blocking:** none known before preflight.

**Non-blocking:** final public result/outcome names; cancellation return shape; post-acceptance version number.

**Require local verification:** worktree cleanliness; exact commands; 190/190 remains current; unpublished changes to preserve.

## 10. New-Session Directives

Read the entire handover first. Inspect live GitHub and the local worktree, compare them with the verified HEAD, and tell Curt briefly whether the handover remains current. Continue from `SM-002`; do not restart architecture planning. Review the complete implementation before acceptance or AgentFlow integration. Preserve the Core-only, same-thread, one-current-state design. Do not ask Curt to repeat contained information. Never instruct the implementer to push. Do not add graphs, timers, threads, GUI, agent types, conversation, or MCP because they may be useful elsewhere. Update AgentFlow documentation if an accepted integration contract changes.
