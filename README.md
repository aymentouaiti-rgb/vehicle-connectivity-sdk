# Vehicle Connectivity SDK

A C++ SDK that gives applications a single, uniform way to talk to a vehicle, whatever the physical link (TCP, serial, Bluetooth, SocketCAN) or a simulator. It handles connections, sessions, timeouts, reconnection, error normalization, logging and metrics, and it deliberately does **not** decode vehicle data: responses are returned as raw messages for the application layer to interpret.

It was developed as the connectivity layer of an automotive diagnostic platform (MVP), within a 4-developer team at Amsys Consulting.

## Features

- **Connector abstraction:** every transport implements the `IConnector` contract. Adding a new connector does not require touching the Core.
- **5 connectors:** Virtual, TCP, Serial, Bluetooth, SocketCAN (see [Status](#status-and-known-limitations) for what is validated).
- **Vehicle sessions:** `VehicleSession` manages the lifecycle (start, send, stop) and per-session statistics.
- **Raw responses:** replies come back as `RawVehicleMessage`, with no business decoding.
- **Resilience:** configurable timeouts per connector, connection-loss detection, automatic reconnection (tested with real connection cuts).
- **Normalized errors:** all failures derive from `ConnectorError` with a stable error code catalogue (`TIMEOUT`, `CONNECTION_LOST`, ...).
- **Safe by default:** a read-only security mode blocks potentially destructive commands unless explicitly disabled.
- **Observability:** structured JSON logs (`StructuredLogger`) and metrics (`Metrics`).
- **Configuration-driven:** switch connector by editing a YAML file, without recompiling.
- **Interface discovery:** TCP, Serial and SocketCAN.

## Architecture

```
vehicle-connectivity-sdk/
  include/vcsdk/
    core/            Contracts (IConnector, RawVehicleMessage, VehicleSession, ...)
    connectors/      Virtual, Tcp, Serial, Bluetooth, SocketCan
    net/             Low-level clients (TCP, serial port)
    retry/           RetryPolicy, ReconnectHelper
    security/        CommandSecurityPolicy (READ_ONLY mode)
    monitoring/      Metrics, RateLimiter
    buffer/          LocalBuffer
    logging/         StructuredLogger
    configuration/   Config (YAML)
    factory/         ConnectorFactory
  src/               Implementations
  tests/
    unit/            Isolated, fast suites
    contract/        Contract scenarios shared by all connectors
    fixtures/        ELM327 simulators (TCP + serial), outside the SDK scope
  examples/          End-to-end demonstrator
  docs/              Guides and diagrams
  config.example.yaml
  CMakeLists.txt
```

Component, class, sequence and state-machine diagrams are in [`docs/diagrammes/`](docs/diagrammes/).

## Getting started

### Requirements

- Linux (the SDK uses POSIX sockets, serial ports, SocketCAN and BlueZ)
- CMake 3.16 or newer
- A C++20-capable compiler (GCC 10+ or Clang 10+)
- Libraries: [yaml-cpp](https://github.com/jbeder/yaml-cpp), BlueZ (`libbluetooth`), [GoogleTest](https://github.com/google/googletest), pthreads
- Python 3 (only to run the ELM327 emulator used by the tests and the demo)

On Debian/Ubuntu:

```bash
sudo apt install build-essential cmake libyaml-cpp-dev libbluetooth-dev libgtest-dev python3
```

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### Run the tests

```bash
./tests/run_tests.sh   # starts the simulators, then runs ctest
```

### Run the demo

```bash
python3 tests/fixtures/elm327_emulator.py 35000 &
./build/demo_virtual_connector
```

## Usage

### Minimal example

```cpp
#include "vcsdk/connectors/virtual/VirtualConnector.hpp"
#include "vcsdk/core/VehicleSession.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

auto connector = std::make_shared<VirtualConnector>("127.0.0.1", 35000);
VehicleSession session("SESSION-001", connector, "VIRTUAL");

session.start();

Command cmd;
cmd.commandId = "CMD-1";
cmd.payload = "010C";  // engine RPM (OBD-II PID)
RawVehicleMessage msg = session.send(cmd);

std::cout << msg.rawPayload << std::endl;  // "410C1AF8"

session.stop();
```

### Recommended: configuration + factory

```cpp
#include "vcsdk/configuration/Config.hpp"
#include "vcsdk/factory/ConnectorFactory.hpp"
#include "vcsdk/core/VehicleSession.hpp"

using namespace vcsdk;

auto config = configuration::loadFromYamlFile("config.yaml");
auto connector = factory::ConnectorFactory::create(config);

core::VehicleSession session("SESSION-001", connector, config.connectorType);
session.start();
// ...
session.stop();
```

This lets you switch between Virtual, TCP, Serial, Bluetooth and SocketCAN by editing `config.yaml` only. See [`config.example.yaml`](config.example.yaml) for a complete example.

### Error handling

All SDK errors derive from `ConnectorError`:

```cpp
try {
    session.send(cmd);
} catch (const core::ConnectorError& e) {
    std::cerr << "Error [" << e.code() << "]: " << e.message() << std::endl;
    // e.code() is e.g. ErrorCode::TIMEOUT, ErrorCode::CONNECTION_LOST, ...
}
```

### Metrics

```cpp
const auto& stats = session.getStatistics();
std::cout << "Commands sent:     " << stats.commandsSent << std::endl;
std::cout << "Responses received: " << stats.messagesReceived << std::endl;
```

Connector-level metrics are available when the connector exposes them (e.g. `VirtualConnector`):

```cpp
auto virtualConn = std::dynamic_pointer_cast<VirtualConnector>(connector);
if (virtualConn) {
    std::cout << virtualConn->getMetrics().report();
}
```

### Security mode

By default the SDK runs in **read-only** mode and blocks potentially destructive commands. To allow them explicitly (e.g. in a controlled test environment):

```cpp
virtualConn->setSecurityMode(vcsdk::security::SdkMode::FULL);
```

> **Warning:** never enable `FULL` mode in production without explicit prior validation.

### Writing a new connector

See [`docs/creer-un-connecteur.md`](docs/creer-un-connecteur.md).

## Testing

15 automated test suites (GoogleTest, run through CTest), all passing:

- 5 contract suites, one per connector (Virtual, TCP, Serial, SocketCAN, Bluetooth)
- 8 unit suites (retry policy, reconnect helper, command security policy, local buffer, rate limiter, metrics, structured logger, config and factory)
- 1 integration suite
- 1 performance suite

TCP and serial tests run against the bundled ELM327 simulators.

## Status and known limitations

| Connector | Status |
|---|---|
| Virtual | Validated against the ELM327 simulator |
| TCP | Validated against the ELM327 simulator |
| Serial | Validated against the ELM327 simulator |
| SocketCAN | Compiles and behaves correctly without an interface (discovery, connection errors); real frame exchange **not yet tested** (needs `vcan0` or a real CAN bus) |
| Bluetooth | Compiles; **no functional test yet** (needs a real adapter) |

Other known limitations:

- Bluetooth discovery (BlueZ inquiry) is not implemented; it is left as an explicit TODO in the code.

## Roadmap

- Real Bluetooth discovery (BlueZ inquiry)
- SocketCAN validation on a machine with `vcan` or a real bus
- Bluetooth validation with a real adapter

## Author

Developed by **Aymen Touaiti** ([LinkedIn](https://www.linkedin.com/in/aymen-touaiti/)) as part of an internship at Amsys Consulting.

## License

Copyright (c) 2026 Aymen Touaiti. All rights reserved.

This code is published for portfolio and evaluation purposes. No permission is granted to use, copy, modify or distribute it without prior written authorization.
