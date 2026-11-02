# Changelog

All notable changes to this driver are listed here. Versions follow
[Semantic Versioning](https://semver.org/). Entries marked **BREAKING** need source changes in
code that used the affected API; together they call for a major version bump at release.

## Unreleased

### Documentation

- The documentation site moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, UART protocol,
  architecture, API guide, configuration, porting, examples, troubleshooting), the Doxygen API reference under
  `/api/`, and every code sample compiled by the host build from `tests/docs/`.
- Added the connector pinout, the measurement timing per `#SETM` mode, the supply current and the firmware 3.55
  erratum from the datasheet. Corrected the default measurement timeout (250 ms, not 300 ms) and the transport
  contract: `read()` must return one complete reply line.

### Fixed

- Optional CRC (`#CRCE 1`): replies end in `": C"` (colon, space, decimal CRC) per data sheet §4.2; the driver
  expected `" :C"`, so every reply with a checksum failed to parse. It now verifies the CRC-16/MODBUS over every
  byte before the colon and returns the new `DriverError::CrcMismatch` on a wrong checksum.

### Deprecated

- `detail::StripOptionalModbusCrcSuffix()`: it removes the suffix without checking it. Use
  `detail::VerifyAndStripCrc()`.

### Added

- `DriverError::CrcMismatch`, `detail::Crc16Modbus()` and `Driver::SetCrcRequired()` / `IsCrcRequired()` to
  reject replies that arrive without the checksum.
- Host unit tests (`HF_FDO2_BUILD_TESTS`) that run the driver against a model of the FDO2-G2 command interface.
