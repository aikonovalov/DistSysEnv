# DistSysEnv

Discrete-event simulator for **message-driven** nodes, a **network** model (delay, loss, partitions), and **Raft** / **hybrid Raft** consensus over a replicated **key–value** store. Used for reproducible chaos-style experiments and metrics.

## Impl

This project was written in barely C++.

## Requirements

- C++20 compiler (Clang or GCC)
- CMake >= 3.14
- [Conan](https://conan.io/) (Catch2 is declared in `conanfile.txt`), **or** Catch2 installed so `find_package(Catch2 CONFIG)` works

## Build (Conan + CMake)

From the repository root:

```bash
mkdir -p build && cd build
conan install .. --output-folder=. --build=missing
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Layout

| Path | Role |
|------|------|
| `src/core/` | MDS core: events, `EventManager`, `NodeID` |
| `src/simulation/` | `SimulationScenario`, `Network`, timers, `SimulationContext` |
| `kv/` | Local `KVStore` template |
| `raft/` | Classic Raft node |
| `hybrid_raft/` | Hybrid Raft with log-backoff / conflict hints |
| `tests/` | Catch2 tests (`test_core`, `test_simulation`, `test_raft`, `test_mc`) |
| `tests/mc/` | Monkey chaos scenarios, quorum schedules, metrics hooks |
| `tests/metrics/` | `RaftMcMetricsCollector`, `BuildRaftMcSimulation` |
| `examples/` | Small demos (`main_example`, `raft_example`) |

## Targets

Libraries: `distsysenv_core`, `distsysenv_simulation`, `distsysenv_raft`, `distsysenv_hybrid_raft`.

Executables:

- `test_core`, `test_simulation`, `test_raft` — unit / integration tests
- `test_mc` - metrics comparsion test
- `main_example`, `raft_example` — examples

## Run tests

```bash
./test_core
./test_simulation
./test_raft
./test_mc
```

## Run examples

```bash
./main_example
./raft_example
```

Catch2 runs as normal binaries.

## Batch metrics (`mc_seed_sweep`)

```text
mc_seed_sweep <first_seed> <count> [jitter|loss|both]
```

Example:

```bash
./mc_seed_sweep 1 50 both > sweep.csv
```

## Coursework

The term paper live under `term_paper/`.
