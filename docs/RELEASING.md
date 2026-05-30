# Releasing Komozoi EVM

## Python Wheels

Komozoi EVM is published to PyPI as binary wheels for x86_64 on Linux.  aarch64 and macOS support are currently disabled.

The release process is automated via GitHub Actions in `.github/workflows/release-pypi.yml`.

### Multi-architecture builds

aarch64 builds are currently disabled.  When enabled, they require QEMU emulation for Linux on x86_64 runners.

### Triggering a release

A release is triggered by:
1. Pushing a tag starting with `v` (e.g., `v0.0.1`) to the `main` or `master` branch.
2. Manually triggering the "Publish Python release to PyPI" workflow via `workflow_dispatch`.

Note: Publishing to PyPI requires the `pypi` environment to be configured with appropriate trusted publishing permissions.
