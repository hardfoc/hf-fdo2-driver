---
layout: default
title: "Troubleshooting"
nav_order: 8
parent: "Documentation"
permalink: /docs/troubleshooting/
---

# Troubleshooting

## No response or garbage data

- Confirm **19200** 8N1 unless you have intentionally reprogrammed the sensor with `#BAUD`.
- Verify **TX/RX are not swapped** and grounds are common.
- Respect **3.3 V max** on the FDO2 UART pins.
- Wait **≥ 1 s** after power-up before the first command.

## `Timeout` or `ProtocolError`

- Increase `SetMeasureTimeoutMs` (default 300 ms; `#MRAW` can be longer at high `#SETM` modes on device).
- Ensure no other task is reading the same UART concurrently without mutual exclusion.
- If **CRC** is enabled on the device, verify you are on a driver revision that strips
  the ` : <CRC>` suffix (current `main` does).

## `DeviceError` after `#ERRO`

Call `LastDeviceErrorCode()` and map **E** using Table 1 in the data sheet §4.4
(for example **-22** UART receive, **-26** unknown header).

## Status **S** not 0 or 1

The data sheet requires checking **S** every sample. If `MoxyStatusOkForOxygen`
returns false, treat pO₂ / temperature as **invalid for safety-critical use** until
the fault clears (see bit definitions in `fdo2::moxy_status`).

## Need more help?

Open an [issue](https://github.com/hardfoc/hf-fdo2-driver/issues) with UART capture,
firmware **R** from `#VERS`, and software revision.

**Next:** [Datasheet and links →](datasheet/README.md)
