# External Regression Files

This folder is for local-only regression files that should not be committed by default.

Use it for private or large files when manually testing:

- EXR plates
- TIFF scans
- JPEG XL files
- broken/corrupt decoder test cases
- camera/vendor-specific images

## Rules

- Do not commit copyrighted or client material.
- Do not make default automated tests depend on this folder.
- If a regression file exposes a bug, create a small generated fixture when possible.
- If a file cannot be shared, document only the observed behavior and file characteristics.

Future optional tests may be enabled through a local config or environment variable.
