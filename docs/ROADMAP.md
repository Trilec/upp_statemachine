# Roadmap

## Near-term

- Event queue policy
- Cancellation policy
- Callback and lifetime safety notes
- GUI demo cleanup

## Current behavior

- `RejectWhileTransitioning` is implemented.
- `DropWhileTransitioning` is implemented.
- `QueueWhileTransitioning` is declared but not fully implemented yet.

### Event queue policy

Queueing is not implemented yet. The options are:

- `DropWhileTransitioning`
- `QueueWhileTransitioning`
- `RejectWhileTransitioning`

Current behavior is:

- `RejectWhileTransitioning`
- `DropWhileTransitioning`

`QueueWhileTransitioning` is declared but not fully implemented yet.

## Later

- Hierarchical states
- StateViewManager
- Code generation helpers
- Formal UppHub packaging notes
