# KvadraAccelerometer — Distributed Accelerometer Processing System

Distributed system for accelerometer data processing implemented in C++17. Three independent processes communicate over TCP with JSON serialization (Level 1).

## Architecture

```
Node A (sender)               Server (Linux)                      Node B (processor)
─────────────────             ──────────────────                  ─────────────────────
SensorEmulator                port 8080 <- Node A                 receives AccelPacket
v 50 Hz           ------->    DuplicateFilter         ------->    computes sqrt(x²+y²+z²)
AccelPacket       <-------    port 8081 <-> Node B    <-------    AccelModule
accel_module.log              forwards AccelModule
```

**Data flow:**
1. Node A generates accelerometer readings at ~50 Hz and sends them to the Server (port 8080).
2. The Server deduplicates consecutive identical readings (precision: 4 decimal places) and forwards unique packets to Node B (port 8081).
3. Node B computes the magnitude `|a| = sqrt(x² + y² + z²)` and sends `AccelModule` back to the Server.
4. The Server relays `AccelModule` to Node A.
5. Node A appends `{timestamp, module}` to `accel_module.log`.

All components auto-reconnect on connection loss (configurable delay, default 3 s).

## Protocol (Level 1)

Each JSON message is terminated with `\n`.

### AccelPacket (Node A -> Server -> Node B)
```json
{"version": 1, "timestamp": 1712938123456, "x": 0.123, "y": 9.807, "z": 0.045}
```

### AccelModule (Node B -> Server -> Node A)
```json
{"version": 1, "timestamp": 1712938123456, "module": 9.808}
```

### Versioning
Every message carries `"version": 1`. A receiver that encounters a different version logs a warning but continues processing (forward compatibility). This allows future protocol evolution without hard breaks.

## Serialisation format comparison
  
| Format        | Size  | Schema   | Human-readable | Build complexity | Choice |
|---------------|-------|----------|----------------|------------------|--------|
| **JSON**      | large | no       | yes            | header-only      | ✓ L1   |
| MessagePack   | small | no       | no             | header-only      |        |
| CBOR          | small | no       | no             | header-only      |        |
| **Protobuf**  | small | .proto   | no             | CMake/gRPC       | ✓ L2   |
| Cap'n Proto   | zero-copy | schema | no           | complex          |        |

JSON was chosen for Level 1 because it is human-readable (easy to debug with `nc` or `tcpdump`), requires no schema compilation, and its overhead is acceptable at 50 Hz with small payloads (~100 B/packet). Protobuf is used in Level 2 for efficiency and schema enforcement. MessagePack and CBOR offer better size than JSON without a schema but lack the tooling ecosystem of Protobuf. Cap'n Proto provides zero-copy parsing but its build complexity is not justified for this workload.

## Build (Linux, Ubuntu 22.04)

### Prerequisites
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git
# CMake >= 3.16 required; Ubuntu 22.04 ships 3.22
```

### Build
```bash
git clone https://github.com/resusfactor228/KvadraAccelerometer
cd KvadraAccelerometer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Or use the helper script:
```bash
./scripts/build.sh
```

Binaries: `build/server/server`, `build/node_a/node_a`, `build/node_b/node_b`.

`nlohmann/json` v3.11.3 is fetched automatically via CMake `FetchContent` — no manual installation required.

## Running

Start components in this order (Server and Node B must be ready before Node A begins sending):

**Terminal 1 — Server:**
```bash
./build/server/server config/server.conf
# Or with explicit ports:
./build/server/server config/server.conf 8080 8081
```

**Terminal 2 — Node B:**
```bash
./build/node_b/node_b config/node_b.conf
```

**Terminal 3 — Node A:**
```bash
./build/node_a/node_a config/node_a.conf
```

Results are appended to `accel_module.log`:
```
1778449986895 9.910464
1778449986915 9.910945
```

### Command-line overrides

```
server  [config]  [port_a]  [port_b]
node_a  [config]  [host]    [port]    [freq_hz]  [log_file]
node_b  [config]  [host]    [port]
```

Example — non-default ports:
```bash
./build/server/server config/server.conf 9090 9091
./build/node_b/node_b config/node_b.conf 127.0.0.1 9091
./build/node_a/node_a config/node_a.conf 127.0.0.1 9090 50.0 out.log
```

## Configuration files

### config/server.conf
```ini
port_a        = 8080   # port for Node A connections
port_b        = 8081   # port for Node B connections
dup_precision = 4      # decimal places for duplicate comparison
```

### config/node_a.conf
```ini
server_host       = 127.0.0.1
server_port       = 8080
sensor_freq_hz    = 50.0        # sampling frequency (Hz)
log_file          = accel_module.log
reconnect_delay_s = 3
```

### config/node_b.conf
```ini
server_host       = 127.0.0.1
server_port       = 8081
reconnect_delay_s = 3
```

## Sensor emulation

Node A emulates accelerometer data with sinusoidal signals that mimic a stationary device with slight vibration:

```
x = 0.5 * sin(2π * 1 Hz * t)
y = 9.81 + 0.1 * cos(2π * 0.5 Hz * t)   # gravity on Y axis
z = 0.2 * sin(2π * 2 Hz * t + 0.5)
```

Relative program time (not epoch seconds) is used for the oscillator to maintain `float` precision. The epoch millisecond timestamp is still recorded in every packet for traceability.

## Project structure

```
KvadraAccelerometer/
├── CMakeLists.txt
├── common/include/
│   ├── logger.hpp          
│   ├── protocol.hpp        
│   ├── tcp_connection.hpp  
│   └── config.hpp          
├── server/src/
│   ├── duplicate_filter.hpp
│   ├── server.hpp / server.cpp
│   └── main.cpp
├── node_a/src/
│   ├── sensor_emulator.hpp
│   ├── node_a.hpp / node_a.cpp
│   └── main.cpp
├── node_b/src/
│   ├── node_b.hpp / node_b.cpp
│   └── main.cpp
├── config/
│   ├── server.conf
│   ├── node_a.conf
│   └── node_b.conf
└── scripts/build.sh
```
