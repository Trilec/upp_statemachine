# LumaPix Development Handbook

This handbook is the day-to-day working guide for Curt, Gary, and coding agents.

## Project roles

## Curt

Curt is the project owner and product/design lead.

Responsibilities:

- decides product direction and milestone priority
- publishes and reviews GitHub work
- evaluates whether the application feels useful for VFX/image workflows
- defines UI/design intent, especially for future viewer/gallery tools
- approves dependency expansion

## Gary

Gary is the implementation lead.

Responsibilities:

- implements C++/U++ changes
- verifies builds and tests locally
- keeps code consistent with U++ style and repo guides
- delegates small scoped tasks to agents where useful
- reports exact build/test results back to Curt

## Coding agents

Agents may implement narrow tasks.

Agents must:

- read `README.md`, `AGENT_GUIDE.md`, `docs/README.md`, and this handbook first
- avoid broad rewrites without approval
- keep third-party SDKs out of the repository
- add/update tests for behavior changes
- keep docs aligned with API changes
- report skipped tests clearly
- never claim external dependency support unless it actually builds locally

## Repository layout expectations

```text
upp_lumepix/
    README.md
    AGENT_GUIDE.md
    THIRD_PARTY
    CHANGELOG.md
    GitHubOut.var

    LumaPix/
        core U++ package

    LumaPixTests/
        test executable package

    LumaPixExamples/
        small example programs

    LumaPixCli/
        command-line inspection tools

    docs/
        LumaPix-specific architecture, milestones, testing, dependency notes

    UPP_GUIDES/
        general U++ and UI-control guides

    testdata/
        tiny legal fixtures and regression-file policy

    scripts/
        build/test/helper scripts

    third_party/
        README only; no vendor SDKs committed
```

## U++ package rules

- Core package names should be unique and stable.
- Main/test/example packages may live alongside the core package in the same nest.
- Keep package dependency direction simple.
- Do not mix unrelated libraries into this nest.
- Put examples in example packages.
- Put automated tests in a dedicated test package.

UppHub expects a module to be a U++ nest and can contain packages, examples, and testing code. Therefore `upp_lumepix` should remain a clean nest rather than a general dumping ground.

## Build/output convention

The repository includes `GitHubOut.var` to document the local build layout.

Current known convention:

```text
UPP = "E:/apps/github/upp_lumepix/examples;E:/apps/github/upp_lumepix;E:/apps/github/upp_AnimationEasing;E:/upp-18468/uppsrc";
OUTPUT = "E:/apps/github/upp_lumepix/out";
```

Policy:

- runnable test/example/CLI executables should be easy to find in `out/`
- intermediate build directories may live under `out/` subfolders
- do not commit generated binaries or intermediate output
- update `AGENT_GUIDE.md` if this convention changes

## Dependency policy

Until the OIIO milestone:

- no OpenImageIO
- no OpenColorIO
- no OpenEXR
- no FFmpeg
- no committed binary SDKs

When dependencies are introduced:

- use an external SDK root such as `VFXSDK_ROOT`
- keep version records in docs/config
- isolate external headers to backend wrappers
- keep the LumaPix public API independent

## Naming discipline

Prefer clear names over short clever names.

Good examples:

```cpp
CalculateImageByteCount()
ValidateImageSpec()
ReadImageMetadata()
ConvertOiioSpecToLumaPixSpec()
BuildDependencyReport()
```

Avoid:

```cpp
DoIt()
Proc()
Fix()
Load2()
imgbuf()
```

## Header comments

Each important public header should have a short file-level comment:

```cpp
// LumaPixImageBuffer.h
// Part of LumaPix.
//
// Purpose:
//   Owns decoded image pixel data in a backend-independent form.
//
// Notes:
//   Must not depend on OpenImageIO, OpenColorIO, FFmpeg, or U++ Image.
//   The viewer/display layer may create a display copy from this buffer.
```

Do not create huge ornamental banners. Make comments useful.

## API change rule

Before `1.0.0`, API changes are allowed.

But every public API change must update:

- tests
- `CHANGELOG.md`
- relevant docs
- version notes if behavior changes

Breaking changes should be made early, especially before OpenImageIO and viewer packages depend on the current names.

## Agent task shape

Good agent task:

```text
Add tests for invalid LumaPixImageSpec values: zero width, negative height, zero channels, unknown sample type. Do not add external dependencies. Update docs if IsValid semantics change.
```

Bad agent task:

```text
Make LumaPix support all VFX image formats.
```

A task is ready for an agent when it has:

- files likely to change
- acceptance criteria
- tests to run
- things explicitly not to change

## Review checklist

Before accepting a commit:

- Does it build?
- Do tests run and fail correctly on failure?
- Did it introduce a third-party dependency accidentally?
- Did it expose backend types through public UI-facing APIs?
- Are names clear?
- Are diagnostics useful?
- Are docs updated?
- Is the implementation smaller than the problem, not bigger?

## Design principle

LumaPix should feel like a carefully engineered VFX foundation, not a random pile of wrappers.

Small, clean, boring foundations are good. The magic comes later when the viewer opens an EXR correctly, displays it through the right color transform, and nobody has to fight the code to understand why.
