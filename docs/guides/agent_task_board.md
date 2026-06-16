# LumaPix Agent Task Board

This file turns the roadmap into small implementation tasks suitable for Gary or coding agents.

## Rules for all tasks

Every task must state:

- goal
- files likely to change
- acceptance criteria
- tests to run
- things not to change

Agents should not combine unrelated tasks.

## Current recommended next phase

Target: `v0.0.2-api-cleanup`

Purpose:

Clean and harden the foundation before OpenImageIO integration.

## Task A — Rename layout/sample terminology

Goal:

Clarify the distinction between channel layout and sample storage type.

Likely files:

- `LumaPix/LumaPixTypes.h`
- `LumaPix/LumaPixTypes.cpp`
- `LumaPix/LumaPixImageSpec.h`
- `LumaPix/LumaPixImageSpec.cpp`
- `LumaPixTests/LumaPixCoreTests.cpp`
- docs/changelog files

Preferred target names:

```cpp
LumaPixChannelLayout
LumaPixSampleType
```

Acceptance:

- old unclear names are removed or temporarily aliased with a clear deprecation note
- tests pass
- docs mention the new names

Do not:

- add OIIO
- add viewer code
- change allocation behavior except where required by naming

## Task B — Define data-window semantics

Goal:

Decide whether `LumaPixDataWindow{0,0,0,0}` means unset/full-image or invalid.

Recommendation:

Use unset/full-image for now.

Likely files:

- `LumaPix/LumaPixTypes.h`
- `LumaPix/LumaPixImageSpec.h`
- `LumaPix/LumaPixImageSpec.cpp`
- `LumaPixTests/LumaPixCoreTests.cpp`

Add methods if useful:

```cpp
bool HasDataWindow() const;
LumaPixDataWindow GetEffectiveDataWindow() const;
```

Acceptance:

- tests prove default data window behavior
- docs explain the behavior

Do not:

- implement EXR display-window logic yet

## Task C — Add checked image byte count

Goal:

Prevent overflow/unsafe allocation once external decoders arrive.

Likely files:

- `LumaPix/LumaPixImageSpec.h`
- `LumaPix/LumaPixImageSpec.cpp`
- `LumaPix/LumaPixImageBuffer.cpp`
- `LumaPixTests/LumaPixCoreTests.cpp`

Suggested API:

```cpp
LumaPixResult LumaPixCalculateImageByteCount(const LumaPixImageSpec& spec, int64& out_bytes);
```

Acceptance:

- valid specs return expected size
- huge specs fail cleanly
- `Allocate()` uses checked helper
- buffer remains empty after failed allocation

Do not:

- silently clamp sizes
- allocate partial buffers

## Task D — Make public headers safer

Goal:

Reduce hidden include-order coupling.

Likely files:

- public headers in `LumaPix/`

Acceptance:

- each public header includes what it directly needs, or there is a documented reason not to
- `LumaPix.h` still works as the normal umbrella include
- tests still build

Do not:

- include external SDK headers

## Task E — Add skipped-test support

Goal:

Prepare the test framework for optional OIIO/OCIO/FFmpeg tests.

Likely files:

- `LumaPixTests/TestFramework.h`
- `LumaPixTests/TestFramework.cpp`
- `LumaPixTests/LumaPixCoreTests.cpp`

Suggested API:

```cpp
void Skip(const String& reason);
#define LPX_SKIP(reason)
```

Expected output:

```text
[PASS] TestName
[SKIP] TestName: reason
SUMMARY passed=6 failed=0 skipped=1
```

Acceptance:

- skipped tests do not fail the test process
- failed tests still return non-zero
- summary includes passed/failed/skipped counts

## Task F — Add dependency-report placeholders

Goal:

Create a stable diagnostic hook before dependencies exist.

Likely files:

- `LumaPix/LumaPixDiagnostics.h`
- `LumaPix/LumaPixDiagnostics.cpp`
- or new `LumaPix/LumaPixDependencies.h/.cpp`
- `LumaPix/LumaPix.upp`
- `LumaPixTests/LumaPixCoreTests.cpp`

Suggested API:

```cpp
String LumaPixGetDependencyReport();
bool LumaPixHasOpenImageIO();
bool LumaPixHasOpenColorIO();
bool LumaPixHasOpenEXR();
bool LumaPixHasFFmpeg();
```

Initial output:

```text
OpenImageIO: disabled
OpenColorIO: disabled
OpenEXR: disabled
FFmpeg: disabled
```

Acceptance:

- report is tested
- no external dependencies are required
- CLI can later reuse the same function

## Task G — Add `CHANGELOG.md` discipline

Goal:

Make version changes traceable from the start.

Likely files:

- `CHANGELOG.md`
- `LumaPix/LumaPixVersion.*`
- tests

Acceptance:

- changelog has `0.0.1` entry
- unreleased section exists
- version tests still pass

## Task H — Prepare `testdata/` policy

Goal:

Prepare for generated and external test data before OIIO arrives.

Likely files:

- `testdata/README.md`
- `testdata/generated/.gitkeep`
- `testdata/external/README.md`

Acceptance:

- repo clearly says what may be committed
- private/copyrighted plates are forbidden
- generated fixture plan is documented

## First OIIO task, not yet

Only after v0.0.2:

```text
Add optional OpenImageIO dependency detection and a reader skeleton that can compile only when OIIO flag is enabled.
```

That should be a separate milestone, not mixed into the cleanup pass.
