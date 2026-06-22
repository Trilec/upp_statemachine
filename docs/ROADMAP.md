# Roadmap

## Near-term

- Event queue policy
- Cancellation policy
- Callback and lifetime safety notes
- GUI demo cleanup

### Event queue policy

Queueing is not implemented yet. The options are:

- `DropWhileTransitioning`
- `QueueWhileTransitioning`
- `RejectWhileTransitioning`

Current behavior is effectively `RejectWhileTransitioning`.

## Later

- Hierarchical states
- StateViewManager
- Code generation helpers
- Formal UppHub packaging notes
