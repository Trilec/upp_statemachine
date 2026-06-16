# LumaPix Test Data

This folder is reserved for small, legal test fixtures.

## Policy

Do commit:

- tiny generated images used by automated tests
- hand-authored metadata samples
- README files that explain test-data expectations

Do not commit:

- studio plates
- copyrighted production images
- private client material
- large media files
- binary SDK payloads

## Planned folders

```text
testdata/generated/
    small generated fixtures safe for source control

testdata/external/
    local-only regression files, not required for default tests
```

## Future generated fixtures

Once OpenImageIO support begins, create tiny fixtures such as:

```text
tiny_rgb_1x1.png
tiny_rgba_2x2.png
tiny_gray_4x4.png
tiny_rgb_2x2.tiff
tiny_float_2x2.exr, if available
```

These should be reproducible by script and small enough that test output remains fast and clear.
