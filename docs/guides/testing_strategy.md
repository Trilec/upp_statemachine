# LumaPix Testing Strategy

## Purpose

LumaPix should be developed test-first wherever practical.

The test suite exists for three reasons:

1. Help Gary verify changes quickly.
2. Help coding agents detect regressions without guessing.
3. Protect the future VFX backend work from breaking the core model.

## Current test state

`v0.0.1` has:

- `LumaPixTests` package
- tiny in-repo registration framework
- command-line runner
- non-zero exit on failure
- baseline tests for result/spec/buffer/metadata/diagnostics/version behavior

That is a good start.

## Test layers

## Layer 1 — pure core tests

These tests must not require external SDKs.

Examples:

- result success/failure behavior
- version string behavior
- image spec validation
- byte-count calculation
- buffer allocation and clearing
- metadata set/get/remove behavior
- diagnostics severity behavior

These should always run.

## Layer 2 — dependency detection tests

These tests verify whether optional dependencies are available.

They should not fail just because a dependency is not configured.

Expected behavior:

```text
[SKIP] OpenImageIO SDK not configured
[SKIP] OpenColorIO SDK not configured
```

A missing optional SDK is not a test failure unless the build configuration explicitly says that SDK is required.

## Layer 3 — generated fixture tests

These use tiny legal test files under `testdata/generated/`.

Target fixtures:

```text
tiny_rgb_1x1.png
tiny_rgba_2x2.png
tiny_gray_4x4.png
tiny_rgb_2x2.tiff
tiny_float_2x2.exr, only when EXR generation is available
```

Generated fixtures should be reproducible by script.

## Layer 4 — external regression tests

These use real-world files that may not be committed.

Location:

```text
testdata/external/
```

Policy:

- never commit copyrighted plates
- never commit studio files
- never require external regression files for default success
- allow a local environment variable or config file to enable them

## Test runner behavior

The runner should print:

```text
Running N tests
[PASS] TestName
[FAIL] TestName
  file:line expected condition
[SKIP] TestName reason
Summary: passed=X failed=Y skipped=Z
```

Current runner prints passes and failures. A future improvement should add explicit skipped count.

## Agent-friendly output

Output should be stable and readable. Agents work better when they can see a plain summary.

Recommended future final line:

```text
SUMMARY passed=12 failed=0 skipped=2
```

## Minimum test set for v0.0.2

Add tests for `LumaPixImageSpec`:

- default spec is not valid
- width zero is invalid
- height zero is invalid
- negative width is invalid
- negative height is invalid
- depth zero is invalid
- channels zero is invalid
- unknown layout/sample type is invalid
- valid UInt8 RGB byte count is correct
- valid UInt16 RGBA byte count is correct
- valid Float32 RGBA byte count is correct
- excessive dimensions fail before allocation

Add tests for `LumaPixImageBuffer`:

- default buffer is empty
- valid allocation stores spec
- valid allocation creates correct byte count
- clear resets data and spec
- invalid spec returns failure
- excessive size returns failure and leaves buffer empty

Add tests for `LumaPixMetadata`:

- missing key returns empty or default value according to documented behavior
- setting same key twice updates value
- key count is stable
- remove/clear behavior if implemented

## Future OIIO tests

When OpenImageIO is added:

- tests must compile only when OIIO feature flag is enabled, or run as skip when disabled
- first tests should use tiny fixtures only
- check metadata and dimensions before checking full pixel values
- keep a direct `--deps` or dependency-report test

Initial OIIO acceptance tests:

```text
Can report OIIO availability
Can open tiny PNG
Can read tiny PNG spec
Can read tiny PNG pixels
Can read basic metadata
Can fail cleanly on nonexistent file
Can fail cleanly on unsupported file
```

## Future viewer tests

GUI testing can be harder, so split viewer testability into two parts:

1. Pure logic tests:
   - zoom math
   - fit-to-window scale
   - frame sequence detection
   - channel-selection state
   - viewer command/state model

2. Manual demo checks:
   - load file
   - zoom/pan
   - metadata panel visible
   - failure message visible

Do not make the viewer untestable by putting all behavior directly inside paint/event functions.

## CI later

For now, local `umk` build/test is enough.

Later, consider GitHub Actions only after:

- command-line build is stable
- external SDK configuration is optional
- tests can run without proprietary files
- Windows build behavior is documented

## Definition of done for a change

A code change is not done until:

- it builds
- relevant tests pass
- new behavior has a test
- failure mode is clear
- docs are updated when public behavior changes

If a change is too hard to test directly, add diagnostics or split the code until the important part can be tested.
