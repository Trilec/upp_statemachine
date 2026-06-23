# Roadmap

## Near-term

- Event queue policy
- Cancellation policy
- Callback and lifetime safety notes
- GUI demo cleanup

## Current behavior

- `RejectWhileTransitioning` is implemented.
- `DropWhileTransitioning` is implemented.
- `QueueWhileTransitioning` is implemented as a bounded FIFO queue of event names for `TriggerEvent()` only.

### Event queue policy

Current options are:

- `DropWhileTransitioning`
- `QueueWhileTransitioning`
- `RejectWhileTransitioning`

Queueing intentionally stays small: bounded, FIFO, event-name only, and single-threaded.

## Later

- Hierarchical states
- StateViewManager
- Code generation helpers
- Formal UppHub packaging notes
