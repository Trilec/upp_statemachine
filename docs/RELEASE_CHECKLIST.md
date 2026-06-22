# Release Checklist

- [ ] Build `StateMachineCoreTest`
- [ ] Run `StateMachineCoreTest`
- [ ] Confirm `Failed: 0`
- [ ] Build `StateMachineGuiTest`
- [ ] Confirm `statemachine/statemachine.upp` uses only `Core`
- [ ] Confirm README include path works
- [ ] Confirm `docs/API.md` matches public header
- [ ] Confirm `CHANGELOG.md` updated
- [ ] Confirm no TODO claims implemented features that are not implemented
- [ ] Confirm event queueing is not documented as implemented
- [ ] Confirm transition cancellation is not documented as implemented
- [ ] Confirm GUI code is only under `examples/`
- [ ] Confirm `statemachine/statemachine.upp` has no `mainconfig`
