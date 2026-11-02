---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# FDO2 Driver

<p>A C++17 driver for the PyroScience FDO2-G2 optical oxygen sensor. It identifies the module, measures oxygen
partial pressure and temperature, reads the raw optical, pressure and humidity data and decodes the status and
error codes, on any MCU with a UART.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-fdo2-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class that writes bytes to a UART and returns one reply
line; it does the rest: the ASCII commands, reply parsing, unit scaling, status bits and `#ERRO` codes.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">Optical oxygen sensor module</span>
<span class="hf-chip__name">FDO2-G2</span>
<span class="hf-chip__vendor">PyroScience</span>
</div>

Measures
:   Oxygen partial pressure 0 to 1000 hPa (0 to 100 % O<sub>2</sub> at 1013 mbar), plus temperature, pressure and
    humidity inside the housing

Accuracy
:   ±5 hPa at 200 hPa (10 °C to 40 °C); response time t<sub>63</sub> under 2 s

Host interface
:   UART, 3.0 V levels (3.3 V absolute maximum), 19200 baud 8N1 after power-up, ASCII commands

Supply
:   3.3 V to 5.0 V DC (5.5 V absolute maximum); about 8 mA standby, 40 mA peak

Connector
:   Molex 560020-0420, four pins: GND, TXD, RXD, VCC

Driver uses
:   `#VERS`, `#IDNR`, `#MOXY`, `#MRAW`, `#LOGO`; no command that writes the module's flash

[Datasheet (v5, 03/2026)](datasheet/Datasheet_FDO2-G2.pdf)
</div>

## What you get

- **One object per sensor.** `fdo2::Driver<YourUart>` sends a command and returns a typed result. See
  [API guide](api-guide.md).
- **Engineering units.** hPa, °C, mbar, mV and %RH, scaled from the module's integer fields, and volume percent
  O<sub>2</sub> from the internal pressure sensor. See [UART protocol](uart-protocol.md#reply-fields).
- **Status you cannot miss.** Every reading carries the status word, and `MoxyStatusOkForOxygen()` applies the
  datasheet's rule. See [API guide](api-guide.md#handle-errors-and-status).
- **Blocking or split.** `MeasureMoxy()` waits for the reply; `SendMeasure()` and `TryReadReplyLine()` let a loop
  serve other devices during the measurement. See [API guide](api-guide.md#measure-without-blocking).
- **Zero-overhead portability.** The port is a CRTP class with three methods: no virtual calls, no heap, no RTOS
  dependency. See [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a model of the sensor's command interface.
  See [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the sensor, write the port, read oxygen in about 15 lines.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Connector pinout, UART settings, timing and power.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Identify, measure, read raw data and handle errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the UART interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and member, generated from the headers.

</div>
