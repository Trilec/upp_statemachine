# LumaPix Documentation Map

This folder contains the project-specific engineering documentation for `upp_lumepix`.

`UPP_GUIDES/` remains the retained U++ and UI-control knowledge base. Use `docs/` for LumaPix-specific architecture, milestones, dependency strategy, testing strategy, and project-management records.

## Recommended reading order

1. `architecture.md`
   - The intended package layers, API boundaries, and backend strategy.
2. `milestones.md`
   - The staged development plan from `v0.0.1` foundation through image loading, viewer preview, OCIO, video, and future graph/MCP work.
3. `guides/development_handbook.md`
   - Daily working rules for Curt, Gary, and coding agents.
4. `guides/testing_strategy.md`
   - How the test framework should evolve and how agents should use it to avoid regressions.
5. `guides/vfx_dependency_strategy.md`
   - How OpenImageIO, OpenColorIO, OpenEXR, FFmpeg, and related SDKs should be introduced without polluting the U++ repo.
6. `guides/code_review_2026_06_16.md`
   - Initial review notes on Gary's `v0.0.1` foundation.

## Documentation ownership

- Curt owns project direction, publishing, naming, visual/UI intent, and release acceptance.
- Gary owns implementation details and build verification unless delegated to an agent.
- Agents may propose changes, but must update tests and documentation when they alter behavior.

## Folder policy

- Keep LumaPix-specific docs here.
- Keep general U++ framework notes in `UPP_GUIDES/`.
- Keep future generated API docs under a separate `docs/api/` folder if/when needed.
- Do not store third-party SDK documentation dumps here unless they are short hand-authored notes. Link to upstream docs instead.
