"""The three layers of the driver: hardware-agnostic core, communication interface, platform port.

Reads: From top to bottom. Your application talks to one fdo2::Driver<UartT> object, whose public calls (listed
from inc/fdo2_driver.hpp) send the FDO2-G2's ASCII commands and decode the replies into readings in hPa, degrees
Celsius, mbar and %RH. The communication interface (UartInterface in inc/fdo2_uart_interface.hpp) is a CRTP base
that forwards bytes to your class. The platform port is the only code you write: a class that writes a command,
returns one reply line and flushes the receive buffer. Below it is the sensor.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/fdo2_driver.hpp"
COMM = "inc/fdo2_uart_interface.hpp"
SKIP = {"GetLineTimeoutMs", "GetMeasureTimeoutMs", "LastDeviceErrorCode", "SetLineTimeoutMs", "SetMeasureTimeoutMs",
        "SetSlowCommandTimeoutMs"}


def data():
    drv = (ROOT / DRIVER).read_text()
    body = drv.split("class Driver {", 1)[1]
    public = body.split("public:", 1)[1].split("private:", 1)[0]
    calls = []
    for name in re.findall(r"^    (?:static )?(?:DriverResult<\w+>|DriverError) (\w+)\(", public, re.M):
        if name not in SKIP and name not in calls:
            calls.append(name)
    comm = (ROOT / COMM).read_text()
    hooks = [n for n in ("write", "read", "flush_rx", "delay_ms_impl") if re.search(rf"\b{n}\(", comm)]
    return {
        "calls": calls,
        "port_hooks": hooks,
        "sources": [f"{DRIVER}:Driver public calls", f"{COMM}:UartInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic FDO2 driver, which uses the CRTP UART "
        "interface, which calls the platform port you write, which drives the UART lines of the PyroScience "
        "FDO2-G2 oxygen sensor.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the interface are the same on every MCU. The port is a small class with three methods.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["incubator control, gas monitoring, data logging ..."], tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, [c + "()" for c in d["calls"]], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="fdo2::Driver<UartT>, inc/fdo2_driver.hpp")
    comm = f.card(0, reg.b + 40, W, "Communication interface",
                  ["UartInterface<Derived>: CRTP base, calls resolved at compile time; the driver above it builds",
                   "the ASCII commands, parses the replies, decodes #ERRO codes and strips the CRC suffix"],
                  tone="brand", accent=True)
    required = [h for h in d["port_hooks"] if h != "delay_ms_impl"]
    lines = ["  ".join(h + "()" for h in required)]
    if "delay_ms_impl" in d["port_hooks"]:
        lines.append("delay_ms_impl() optional")
    port = f.card(0, comm.b + 40, W, "Platform port (your class)", lines, tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "FDO2-G2",
                ["UART 19200 baud 8N1, 3.0 V levels  /  connector pins GND, TXD, RXD, VCC"], tone="slate")
    for a, b in ((app, reg), (reg, comm), (comm, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
