# LumaPix Scripts

This folder is for small helper scripts used by Curt, Gary, and coding agents.

## Planned scripts

```text
run_tests.ps1
    Build and run LumaPixTests through umk.

build_all.ps1
    Build core tests, CLI, examples, and later viewer.

make_test_images.py
    Generate tiny legal image fixtures for testdata/generated/.

dump_env.ps1
    Print relevant U++/LumaPix build configuration.
```

## Policy

- Scripts should not assume third-party SDKs unless clearly named.
- Scripts should print useful diagnostics.
- Scripts should fail with non-zero exit when the requested build/test fails.
- Scripts should not copy binaries into Git.
- Keep `GitHubOut.var` as the documented local build/output reference.
