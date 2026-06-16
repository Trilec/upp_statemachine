# VFX Dependency Strategy

## Goal

LumaPix should eventually use serious VFX libraries without becoming hard to build, hard to package, or hard to reason about.

The guiding rule:

> The repository contains LumaPix integration code, not third-party SDK payloads.

## Why not vendor everything?

Do not commit OpenImageIO, OpenColorIO, OpenEXR, FFmpeg, JPEG XL, TIFF, PNG, JPEG libraries, or prebuilt binary SDKs into this repo.

Reasons:

- repository size stays small
- license tracking stays clear
- SDK versions can be upgraded independently
- U++ package structure stays understandable
- UppHub publishing stays cleaner
- coding agents do not accidentally rewrite or damage vendor code

## External SDK shape

Use a local SDK root outside the repo.

Example:

```text
E:/dev/vfxsdk/2026.1/
    include/
    lib/
    bin/
    licenses/
    versions.json
```

Alternative package-organized shape:

```text
E:/dev/vfxsdk/2026.1/
    OpenImageIO/
        include/
        lib/
        bin/
    OpenColorIO/
        include/
        lib/
        bin/
    OpenEXR/
        include/
        lib/
        bin/
    Imath/
        include/
        lib/
    ffmpeg/
        bin/
        licenses/
    versions.json
```

For the first integration, the flat `include/lib/bin` layout is usually easier.

## Version record

Keep a machine-readable record outside or beside the SDK:

```json
{
  "sdk_name": "lumapix-vfxsdk",
  "sdk_version": "2026.1",
  "target": "VFX Reference Platform CY2026 where practical",
  "libraries": {
    "OpenImageIO": "3.x",
    "OpenEXR": "3.x",
    "Imath": "3.x",
    "OpenColorIO": "2.5.x",
    "FFmpeg": "external executable first"
  }
}
```

Do not hard-code this in random `.cpp` files. Provide a dependency-report function and CLI command later.

## U++ integration pattern

U++ `.upp` packages support `include`, `library`, `link`, `options`, `mainconfig`, and conditional flags.

The future OIIO package should use a feature flag such as:

```text
mainconfig
    "" = "CONSOLE",
    "With OIIO" = "CONSOLE OIIO";
```

Conceptual package rules:

```text
include(OIIO) "$(VFXSDK_ROOT)/include";
library(OIIO WIN32) OpenImageIO, OpenImageIO_Util;
```

Exact environment-variable expansion and library syntax must be verified in the local U++/umk setup before being treated as final.

## Recommended dependency order

## 1. OpenImageIO

Use first.

Purpose:

- still-image reading
- format abstraction
- metadata
- EXR/TIFF/PNG/JPEG/JPEG XL access depending on build
- future thumbnails/subimages/MIP-levels where useful

LumaPix should wrap only what it needs first:

- open file
- read dimensions/spec
- read metadata
- read pixels
- close file
- report errors

Do not expose OIIO types outside the backend wrapper.

## 2. OpenEXR / Imath

Use through OpenImageIO first.

Add direct OpenEXR only when needed for:

- multipart inspection
- deep-image diagnostics
- compression/channel/layer tools
- EXR-specific repair or validation utilities

Do not make direct OpenEXR the first general image loading path.

## 3. OpenColorIO

Add after basic image loading and viewer preview.

Purpose:

- scene-linear to display transform
- ACES/view/display handling
- studio OCIO configs
- LUT and color pipeline support

Initial OCIO path can be CPU-based. GPU display can come later.

## 4. FFmpeg

Add after still image viewer/gallery foundations.

Initial strategy:

- call FFmpeg as an external executable
- use it to extract thumbnails/frames or generate proxies
- avoid static linking until licensing and redistribution are explicitly reviewed

This keeps video codec complexity away from the core library.

## 5. JPEG XL / PNG / TIFF / JPEG libraries

Prefer using them through OpenImageIO first.

Add direct wrappers only if there is a clear reason, such as:

- faster thumbnails
- very small dependency mode
- a format feature not exposed well through OIIO

## Lessons from similar VFX-style projects

## Keep backends replaceable

OpenFX plugins such as openfx-io have historically combined OpenImageIO, FFmpeg, and OpenColorIO-style roles. That validates the library choices, but LumaPix should avoid mixing those concerns in one giant class.

Separate:

- image file read/write
- video/proxy read/write
- color transform
- UI display
- metadata/model

## Do not start with a node graph

A Nuke-like tool requires an evaluation engine, graph model, cache, scheduler, image regions, transforms, and UI. LumaPix should first become a trustworthy image foundation.

## Preserve high dynamic range

Do not convert source data to 8-bit display pixels until the display step.

The core buffer must support:

- UInt8
- UInt16
- Float16
- Float32
- alpha
- values outside 0..1
- multi-channel/AOV-style images later

## Security and robustness

Image and video decoders parse hostile input. Future decoder code must:

- validate dimensions before allocation
- cap maximum allocation size
- catch exceptions
- convert backend errors to LumaPix diagnostics
- avoid UI crashes from bad files
- include broken-file tests where possible

## Proposed package split when dependencies arrive

Option A: keep backend files in `LumaPix` with conditional flags.

Pros:

- simple early setup
- fewer packages

Cons:

- core package gets cluttered
- optional dependencies can become confusing

Option B: separate backend packages.

```text
LumaPix
LumaPixOiio
LumaPixOcio
LumaPixVideo
```

Pros:

- clean dependency boundaries
- easier UppHub story
- core stays pure

Cons:

- more packages to manage

Recommendation:

- `v0.0.x`: core only
- `v0.1.x`: create `LumaPixOiio` if U++ package wiring is manageable; otherwise start with clearly isolated `backend_oiio` files inside `LumaPix`
- before `v1.0.0`: prefer separate packages if optional SDK support is substantial

## Build verification checklist for first OIIO attempt

Before merging OIIO integration:

- build without OIIO still works
- build with OIIO works locally
- missing SDK gives a clear compile or runtime message
- tests skip cleanly when OIIO is absent
- OIIO headers are not included by viewer/example packages
- dependency report prints configured include/lib/bin paths
- README explains setup in one place

## Licenses

Keep `THIRD_PARTY` updated as dependencies arrive.

At minimum, record:

- library name
- version
- license
- source URL
- how it is linked or invoked
- whether binaries are redistributed

FFmpeg requires extra care because build options can change license obligations.

## Final policy

LumaPix should be able to say:

> Core builds with only U++.
> VFX backends build when a compatible external SDK is configured.
> The public API remains LumaPix, not OpenImageIO or OpenColorIO.
