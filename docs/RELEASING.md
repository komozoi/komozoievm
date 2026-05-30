# Releasing Komozoi EVM

## Python Wheels

Komozoi EVM is published to PyPI as binary wheels for multiple architectures (x86_64 and aarch64) on Linux. macOS support is currently disabled.

The release process is automated via GitHub Actions in `.github/workflows/release-pypi.yml`.

### Multi-architecture builds

To build `aarch64` wheels on `x86_64` GitHub Actions runners, we use `cibuildwheel` which requires QEMU emulation for Linux.  The workflow is optimized to run builds for different architectures in parallel across multiple jobs.

The Linux aarch64 job must include the `docker/setup-qemu-action` step before running `cibuildwheel`:

```yaml
- name: Set up QEMU
  uses: docker/setup-qemu-action@v3
```

### Triggering a release

A release is triggered by:
1. Pushing a tag starting with `v` (e.g., `v0.0.1`) to the `main` or `master` branch.
2. Manually triggering the "Publish Python release to PyPI" workflow via `workflow_dispatch`.

Note: Publishing to PyPI requires the `pypi` environment to be configured with appropriate trusted publishing permissions.
