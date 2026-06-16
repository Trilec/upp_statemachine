# Code Review — 2026-06-16

Review target: initial `v0.0.1` LumaPix foundation published by Gary.

Reviewer: ChatGPT, using GitHub connector access.

## Overall verdict

The initial foundation is good.

Gary kept the scope restrained, which is exactly what this project needs at the start. The repository has a core package, test package, examples/CLI placeholders, root docs, and a no-external-SDK policy. That is the correct base before attempting OpenImageIO or OpenColorIO integration.

No urgent rewrite is recommended.

However, a few design details should be tightened in `v0.0.2` before the API becomes more expensive to change.

## What looks good

## Scope control

`v0.0.1` does not pull in OpenImageIO, OpenColorIO, OpenEXR, FFmpeg, or other external SDKs. This is the right decision.

The project should first prove its own data model, tests, diagnostics, and build conventions.

## Package structure

The current packages are sensible:

- `LumaPix`
- `LumaPixTests`
- `LumaPixExamples`
- `LumaPixCli`

This matches the intended U++ nest structure and keeps the future UppHub story clean.

## Test framework

The tiny test framework is a good first move.

Strengths:

- tests register themselves
- test names are printed
- failures are collected
- the process returns non-zero when failures occur

This is agent-friendly because an agent can run the test package and see simple pass/fail output.

## Result object

`LumaPixResult` is a good abstraction for avoiding silent failures.

Keep this pattern as the project integrates third-party decoders.

## Image buffer ownership

`LumaPixImageBuffer` owning a `Vector<byte>` is a reasonable first foundation. It keeps decoded pixels separate from U++ display classes and prevents an early dependency on GUI/image rendering.

## Items to fix or decide in v0.0.2

## 1. Clarify naming: layout vs sample type

Current code:

```cpp
enum class LumaPixPixelFormat {
    Unknown = 0,
    Gray,
    RGB,
    RGBA
};

enum class LumaPixChannelType {
    Unknown = 0,
    UInt8,
    UInt16,
    Float16,
    Float32
};
```

Issue:

`Gray`, `RGB`, and `RGBA` are not really pixel formats. They describe channel layout. `UInt8`, `UInt16`, `Float16`, and `Float32` describe sample/storage type.

Recommended before OpenImageIO integration:

```cpp
enum class LumaPixChannelLayout {
    Unknown = 0,
    Gray,
    GrayAlpha,
    RGB,
    RGBA,
    MultiChannel
};

enum class LumaPixSampleType {
    Unknown = 0,
    UInt8,
    UInt16,
    Float16,
    Float32
};
```

This will make future mapping from OIIO much clearer.

## 2. Define data-window semantics

Current `LumaPixDataWindow` defaults to zero width/height:

```cpp
struct LumaPixDataWindow {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};
```

Current `IsValid()` only rejects negative data-window width/height.

Decision needed:

Option A:

```text
zero data window means unset, use full image dimensions
```

Option B:

```text
data window must be explicit and positive for valid image specs
```

Recommendation:

Use Option A for simplicity in early milestones, but document it and add helper methods:

```cpp
bool HasDataWindow() const;
LumaPixDataWindow GetEffectiveDataWindow() const;
```

EXR-style windows can become important later, but the first reader should not make every PNG/JPEG manually fill data-window values.

## 3. Add checked allocation math

Current code calculates:

```cpp
GetPixelCount() * GetBytesPerPixel()
```

Then `Allocate()` rejects byte counts below zero or above `INT_MAX`.

This is a good start, but before reading untrusted image files, add checked multiplication helpers so huge dimensions fail deliberately rather than relying on overflow behavior.

Suggested helper:

```cpp
LumaPixResult CalculateImageByteCount(const LumaPixImageSpec& spec, int64& out_bytes);
```

Acceptance tests:

- very large width fails
- very large height fails
- very large channel count fails
- result message explains allocation cap

## 4. Consider self-contained public headers

Current public headers rely on `LumaPix.h` include ordering.

That is common in U++ projects, but before external wrappers arrive, consider making headers include the other LumaPix headers they directly require.

Example:

`LumaPixImageBuffer.h` directly uses:

- `LumaPixImageSpec`
- `LumaPixResult`
- `Vector<byte>`

So it could include:

```cpp
#include <Core/Core.h>
#include "LumaPixResult.h"
#include "LumaPixImageSpec.h"
```

This makes the code easier for agents and external users.

## 5. Add skip support to the test framework

Before optional SDK tests arrive, add explicit skip reporting.

Suggested output:

```text
[PASS] TestName
[FAIL] TestName
[SKIP] TestName: OpenImageIO SDK not configured
SUMMARY passed=6 failed=0 skipped=1
```

This will be important for OIIO/OCIO/FFmpeg tests.

## 6. Add dependency report placeholders

Before adding OpenImageIO, add placeholders:

```cpp
String LumaPixGetDependencyReport();
bool LumaPixHasOpenImageIO();
bool LumaPixHasOpenColorIO();
bool LumaPixHasOpenEXR();
bool LumaPixHasFFmpeg();
```

Initial output can be:

```text
OpenImageIO: disabled
OpenColorIO: disabled
OpenEXR: disabled
FFmpeg: disabled
```

Later, this becomes very useful for debugging SDK configuration.

## 7. Add file-level purpose comments

The code is small and readable, but public headers should explain their purpose.

Suggested style:

```cpp
// LumaPixImageSpec.h
// Part of LumaPix.
//
// Purpose:
//   Describes decoded image dimensions, channel layout, sample type, and
//   optional data-window information without depending on any backend SDK.
```

Keep comments short and useful.

## Proposed v0.0.2 task list for Gary

1. Rename or alias layout/sample terminology.
2. Define and test data-window semantics.
3. Add checked byte-count calculation.
4. Make public headers more self-contained.
5. Add skipped-test support.
6. Add dependency-report placeholders.
7. Add invalid-spec tests.
8. Update `CHANGELOG.md` and docs.

## Do not do yet

Do not add:

- OpenImageIO
- OpenColorIO
- OpenEXR direct API
- FFmpeg
- GUI viewer
- thumbnail cache
- node graph

The foundation still needs one cleanup pass first.

## Final note

The current work is a good first commit. The next pass should be a careful naming and robustness pass, not a feature sprint.

That keeps the first OIIO integration clean instead of bolting serious VFX plumbing onto slightly fuzzy core terms.
