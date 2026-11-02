# FDO2 Driver API reference {#mainpage}

The classes, functions and types of the HardFOC FDO2 driver for the PyroScience FDO2-G2 optical oxygen sensor,
generated from the headers.

Start with fdo2::Driver, the command client, and fdo2::UartInterface, the interface your platform port implements.
Readings are fdo2::MoxyReading and fdo2::MrawReading; every call returns an fdo2::DriverResult that carries an
fdo2::DriverError. The status bits of a measurement are in the fdo2::moxy_status namespace.

The guides, with examples that compile, are on the documentation site: <a href="../">FDO2 Driver
documentation</a>.
