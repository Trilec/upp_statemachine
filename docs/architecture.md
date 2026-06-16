# LumaPix Architecture

## Purpose

`LumaPix` is a U++/C++ image-foundation library intended to support future VFX-oriented tools such as an image viewer, image gallery, sequence browser, metadata inspector, and eventually a color-managed review/viewer application.

The first design rule is simple:

> U++ UI code must not be directly coupled to third-party VFX libraries.

OpenImageIO, OpenColorIO, OpenEXR, FFmpeg, JPEG XL, PNG, TIFF, and other libraries should be treated as replaceable backends behind clean LumaPix interfaces.

## Current status

`v0.0.1` is the foundation stage.

Current packages:

- `LumaPix` — core library package.
- `LumaPixTests` — console test runner and tiny in-repo test framework.
- `LumaPixExamples` — placeholder package for small examples.
- `LumaPixCli` — placeholder package for command-line tools.

Planned packages:

- `LumaPixViewer` — GUI viewer package once image loading is stable.
- `LumaPixOiio` or internal OIIO backend files — OpenImageIO integration.
- `LumaPixOcio` — OpenColorIO display-transform integration.
- `LumaPixVideo` — FFmpeg/proxy/video bridge, initially likely as an external-process bridge.

## Package layering

Recommended dependency direction:

```text
LumaPixViewer / LumaPixExamples / LumaPixCli
    -> LumaPix public API
        -> backend adapters
            -> external SDK libraries
```

Never reverse this direction.

Bad:

```text
viewer directly includes OpenImageIO headers
viewer directly parses EXR metadata
viewer directly manages OCIO processor state
```

Good:

```text
viewer asks LumaPix to load an image preview
viewer asks LumaPix to expose metadata
viewer asks LumaPix to apply a display transform
```

## Core data model

The stable concepts should be:

- image dimensions
- channel layout
- sample/storage type
- data window / display window
- metadata
- pixel buffer ownership
- diagnostics
- result/error reporting

The current implementation already has:

- `LumaPixResult`
- `LumaPixImageSpec`
- `LumaPixImageBuffer`
- `LumaPixMetadata`
- `LumaPixDiagnostics`
- `LumaPixVersion`

## Important naming note before API hardening

The current `LumaPixPixelFormat` enum uses values such as `Gray`, `RGB`, and `RGBA`. That is really a channel-layout concept, not a pixel storage format.

Before OpenImageIO integration, consider renaming:

```text
LumaPixPixelFormat  -> LumaPixChannelLayout
LumaPixChannelType  -> LumaPixSampleType
```

Suggested target model:

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

This will make the OpenImageIO mapping much clearer because OIIO separates channel count/names from the per-channel data type.

## Header policy

For a U++ package, an umbrella header such as `LumaPix.h` is normal and useful. However, headers should still become as self-contained as practical before the API grows.

Preferred rule for new code:

- public headers include the other LumaPix headers they directly need
- `.cpp` files include `LumaPix.h` unless there is a reason to keep compile scope narrow
- external SDK headers are included only by backend implementation files, not by the public core model

## Backend policy

Backends should be optional until they are required by a specific milestone.

Recommended future structure:

```text
LumaPix/
    core files
    backend_oiio/
        LumaPixOiioReader.h
        LumaPixOiioReader.cpp
    backend_ocio/
        LumaPixOcioTransform.h
        LumaPixOcioTransform.cpp
```

or, if the U++ package file becomes too cluttered:

```text
LumaPix/          core only
LumaPixOiio/      OIIO adapter package
LumaPixOcio/      OCIO adapter package
LumaPixVideo/     FFmpeg/video bridge package
```

The second option is cleaner for UppHub if the external dependencies become optional modules.

## Image loading boundary

The future image-loading API should look like LumaPix, not like OIIO:

```cpp
class LumaPixImageReader {
public:
    LumaPixResult Open(const String& path);
    LumaPixResult ReadSpec(LumaPixImageSpec& out_spec) const;
    LumaPixResult ReadMetadata(LumaPixMetadata& out_metadata) const;
    LumaPixResult ReadImage(LumaPixImageBuffer& out_image) const;
    void Close();
};
```

The implementation may use OpenImageIO internally, but the caller should not need to know that.

## Display boundary

Do not collapse the internal image buffer to an 8-bit U++ `Image` too early.

The future display path should be:

```text
source pixels
    -> optional normalization / channel selection
    -> exposure / gain / gamma preview controls
    -> OCIO display/view transform
    -> 8-bit or GPU display buffer
    -> U++ viewer control
```

VFX files can contain half-float or float values, negative values, alpha, values above 1.0, many channels, and non-display color spaces. The core buffer must preserve that reality.

## Diagnostics boundary

All external decoder failures must be converted to LumaPix diagnostics and results.

A good failure message should include:

- operation attempted
- source path
- backend used
- upstream error message if available
- suggested next action if obvious

Example:

```text
OpenImageIO failed to open plate.0001.exr: unsupported compression method. Check SDK/plugin build and file integrity.
```

## Versioning

The project starts at `0.0.1`.

Recommended version meaning:

- `0.0.x` — foundation/internal scaffolding changes.
- `0.1.x` — first image loading through OpenImageIO.
- `0.2.x` — command-line inspection and metadata tooling.
- `0.3.x` — first GUI preview viewer.
- `0.4.x` — color-managed viewing through OpenColorIO.
- `1.0.0` — stable core API and viewer suitable for external use.

Until `1.0.0`, API changes are allowed, but each breaking change must be documented in `CHANGELOG.md` and covered by tests.

## Non-goals for the foundation stage

Do not build these yet:

- Nuke clone
- node graph
- deep EXR editing
- GPU compositor
- full FFmpeg integration
- custom color-management engine
- plugin host
- database-backed asset manager

Those may come later, but only after the basic image model, test framework, and dependency strategy are solid.
