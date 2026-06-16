# LumaPix Milestones

This document defines the staged path for `upp_lumepix`.

The principle is:

> Build the smallest strong foundation first, then add serious VFX dependencies behind narrow interfaces.

## Milestone table

| Milestone | Name | Main result |
|---|---|---|
| `v0.0.1` | Foundation | Core package, tiny tests, docs, no external SDK |
| `v0.0.2` | API cleanup | Harden naming, self-contained headers, safer size checks |
| `v0.1.0` | OIIO read | OpenImageIO-backed image read and metadata dump |
| `v0.1.1` | Test images | Generated fixtures and regression-file policy |
| `v0.2.0` | CLI inspect | `LumaPixCli --info`, `--metadata`, diagnostics |
| `v0.3.0` | Viewer preview | First U++ GUI image viewer control |
| `v0.4.0` | OCIO display | OpenColorIO config/display/view transform support |
| `v0.5.0` | Sequences/gallery | Frame-sequence detection, thumbnails, browser basics |
| `v0.6.0` | Video bridge | FFmpeg external-process bridge for thumbnails/proxies |
| `v1.0.0` | Stable core | Stable public API, documented SDK profile, examples/tests |

## v0.0.1 — Foundation

Status: implemented by Gary and published.

Scope:

- repository skeleton
- `LumaPix` package
- `LumaPixTests` package
- tiny test framework
- `LumaPixExamples` placeholder
- `LumaPixCli` placeholder
- core result/spec/buffer/metadata/diagnostics/version types
- no OpenImageIO
- no OpenColorIO
- no third-party SDKs committed

Acceptance:

- `LumaPix` builds
- `LumaPixTests` builds
- tests run from command line
- test failures return non-zero
- README and dependency policy exist

## v0.0.2 — API cleanup before external dependencies

Purpose:

Clean the foundation before it becomes expensive to change.

Tasks:

1. Rename or clearly alias terminology:
   - `LumaPixPixelFormat` currently means layout; prefer `LumaPixChannelLayout`.
   - `LumaPixChannelType` currently means sample/storage type; prefer `LumaPixSampleType`.
2. Decide exact data-window semantics:
   - zero data window means unset/full image, or
   - data window must be explicitly positive.
3. Add checked-size helper:
   - safe multiplication for width × height × depth × channels × bytes-per-channel.
   - fail before allocation overflow.
4. Make public headers self-contained enough for direct inclusion.
5. Add tests for invalid specs:
   - zero width
   - negative width
   - zero channels
   - unknown sample type
   - excessive allocation size
   - data-window mismatch if applicable
6. Add dependency-report placeholder:
   - `OpenImageIO: disabled/not configured`
   - `OpenColorIO: disabled/not configured`
   - `OpenEXR: disabled/not configured`
   - `FFmpeg: disabled/not configured`

Acceptance:

- all v0.0.1 tests still pass
- renamed/aliased terminology is documented
- invalid image specs are covered by tests
- no external SDK required

## v0.1.0 — OpenImageIO read backend

Purpose:

Read still images through OpenImageIO while keeping OIIO types behind the LumaPix API.

Tasks:

1. Add SDK detection strategy.
2. Add `LumaPixOiioReader` or separate `LumaPixOiio` package.
3. Map OIIO image spec to `LumaPixImageSpec`.
4. Read metadata into `LumaPixMetadata`.
5. Read pixels into `LumaPixImageBuffer`.
6. Preserve useful channel names.
7. Convert upstream errors to `LumaPixResult` and `LumaPixDiagnostics`.
8. Add tests that skip cleanly if OIIO is not configured.
9. Add tiny generated test images.

Initial formats:

- PNG
- JPEG
- TIFF
- EXR, only if the configured SDK supports it

Acceptance:

- build succeeds without OIIO when OIIO feature flag is disabled
- build succeeds with OIIO when SDK paths are configured
- tests can report skip instead of fail when SDK is missing
- no OIIO headers leak into viewer/example code

## v0.1.1 — Test image fixtures and regression policy

Purpose:

Make tests useful for humans and agents.

Tasks:

1. Add generated tiny fixtures under `testdata/generated/`.
2. Add `scripts/make_test_images.py` or equivalent.
3. Add `testdata/external/README.md` explaining private/non-committed production samples.
4. Add tests for:
   - 1×1 RGB PNG
   - 2×2 RGBA PNG
   - grayscale PNG
   - tiny TIFF
   - tiny EXR if available
5. Add expected metadata snapshots for simple files.

Acceptance:

- fixtures are small and legal to commit
- generated fixtures are reproducible
- external regression files are documented but not required

## v0.2.0 — CLI inspection tool

Purpose:

Create a tool useful to Curt, Gary, agents, and later MCP wrappers.

Commands:

```text
LumaPixCli --version
LumaPixCli --deps
LumaPixCli --info path/to/image.exr
LumaPixCli --metadata path/to/image.exr
LumaPixCli --self-test
```

Future commands:

```text
LumaPixCli --thumbnail input.exr output.png
LumaPixCli --compare-metadata a.exr b.exr
LumaPixCli --validate path/to/file
```

Acceptance:

- CLI returns non-zero on real failure
- missing SDK produces clear diagnostics
- output is readable by humans and simple for agents to parse

## v0.3.0 — First U++ viewer preview

Purpose:

Load an image through LumaPix and view it in a U++ GUI.

Features:

- file open
- image preview
- fit to window
- 1:1 zoom
- pan
- metadata side panel
- basic error panel

Important restriction:

The viewer may convert to displayable 8-bit RGBA, but the internal source buffer must remain independent of U++ `Image`.

Acceptance:

- viewer compiles separately
- viewer uses LumaPix API, not OIIO directly
- image load failure is shown clearly
- basic metadata is visible

## v0.4.0 — OpenColorIO display transform

Purpose:

Make the viewer VFX-useful by supporting color-managed display.

Features:

- load OCIO config
- list displays/views
- choose input color space
- choose display/view
- exposure/gamma preview controls
- apply CPU display transform initially

Acceptance:

- OCIO is isolated behind LumaPix wrapper API
- missing config is handled cleanly
- default fallback display path still works

## v0.5.0 — Sequences and gallery

Purpose:

Move from single-file viewer toward a production-style plate browser.

Features:

- detect image sequences
- parse frame numbers
- next/previous frame
- simple playback scrub
- thumbnail cache
- contact sheet / gallery view

Acceptance:

- sequence detection has tests
- missing frames are reported
- large folders do not freeze the UI unnecessarily

## v0.6.0 — Video bridge

Purpose:

Support MOV/MP4/proxy workflows without dragging FFmpeg complexity into the core too early.

Initial recommendation:

Use FFmpeg as an external process first.

Features:

- detect FFmpeg executable
- extract thumbnail/frame
- generate proxy movie from image sequence
- inspect video metadata if practical

Acceptance:

- FFmpeg absence is cleanly reported
- no static FFmpeg linking until licensing is deliberately reviewed
- video failures do not crash the viewer

## v1.0.0 — Stable public core

Purpose:

A stable, documented, reusable U++ VFX image foundation.

Requirements:

- documented public API
- tests for core image model
- tests for OIIO-backed reading
- CLI examples
- viewer example
- dependency profile documented
- no known external SDK leakage into UI packages
- license/third-party notes complete
- UppHub submission checklist complete

## Release discipline

Every milestone should update:

- `CHANGELOG.md`
- `LumaPixVersion`
- tests
- relevant docs

A milestone is not done because it compiles once. It is done when the expected tests, docs, and diagnostics exist.
