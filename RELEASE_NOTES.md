# Release Notes - v0.0.2

This release focuses on fixing a critical arithmetic bug in the EVM simulator by updating the underlying `libexcessive` library.

## Bug Fixes

- **EVM Arithmetic**:
  - Updated `libexcessive` to fix a severe bug in `_internalBigintFastDiv` where dividing by 1 would corrupt values by zeroing the second 64-bit limb.

## Testing

- **EVM Tests**:
  - Added `EVMTest.TestDivByOne` regression test to the `evm_tests` suite to prevent future arithmetic regressions.
