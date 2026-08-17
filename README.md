# SSTA-X

**GPU-accelerated first-order block-based statistical static timing analysis.**

SSTA-X analyzes circuit timing under process variation by propagating first-order
canonical delay models through a levelized timing graph on the GPU. First-order
block-based SSTA (FB-SSTA) approximates timing distributions analytically by
propagating statistical moments. SSTA-X maps the entire propagation onto GPU kernels.


This is the reference implementation for:

> C.-C. Chang, Y.-H. Chung, W. L. Lee, B. Zhang, and T.-W. Huang,
> "SSTA-X: GPU-Accelerated First-Order Block-Based Statistical Static Timing
> Analysis," *IEEE Transactions on Computer-Aided Design of Integrated Circuits
> and Systems (TCAD)*, 2026. doi:
> [10.1109/TCAD.2026.3696812](https://doi.org/10.1109/TCAD.2026.3696812)

## How it works

SSTA-X reads a Liberty library, a Verilog netlist, SPEF parasitics, and a set of
timing assertions, then runs three stages:

1. **Levelization.** A breadth-first traversal assigns each instance a
   topological level. Every instance in a level is independent and can be
   processed concurrently. Nodes with multiple fan-in arcs take the maximum
   level among their fan-ins, so all predecessors are resolved first.
2. **Interconnect timing.** RC trees from the SPEF are traversed on the GPU
   (`ssta/cuda/rct.cu`) to compute load capacitance, Elmore delay, and slew at
   every tap.
3. **Forward and backward propagation.** Arrival times and slews propagate
   forward, then required arrival times propagate backward with setup/hold
   constraints applied at sequential elements (`ssta/cuda/timing.cu`). Each
   timing quantity carries a nominal value plus six variation sensitivities and
   an independent random term, evaluated across four timing modes
   (early/late × rise/fall).

## Requirements

- CUDA Toolkit 11.0 or newer (developed against 12.6)
- CMake 3.18 or newer
- A C++17 compiler with OpenMP
- An NVIDIA GPU; the paper's results were collected on an RTX A4000 (sm_86)

## Building

```bash
git clone --recurse-submodules https://github.com/chih-chun-chang/ssta-x.git
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

If you already cloned without `--recurse-submodules`, fetch Parser-SPEF with
`git submodule update --init --recursive`.

The build targets `sm_86` by default. For a different GPU, pass the compute
capability:

```bash
cmake -S . -B build -DCMAKE_CUDA_ARCHITECTURES=90
```

## Running

`ssta-x` takes a cell library plus the three per-design files. Since a design
directory is named after its design, `--design` derives all three:

```bash
./build/ssta-x -l benchmark/tau2013_contest_library.v1.0.lb -d benchmark/tau2015/c17
```

which is shorthand for:

```bash
./build/ssta-x -l benchmark/tau2013_contest_library.v1.0.lb -v benchmark/tau2015/c17/c17.v -s benchmark/tau2015/c17/c17.spef -t benchmark/tau2015/c17/c17.timing
```

It prints a runtime breakdown of levelization, host-to-device transfer, RC tree
computation, and forward/backward propagation.

To measure propagation runtime, `--benchmark N` rebuilds the timer from scratch
N times and reports the mean forward and backward times, excluding parsing and
setup:

```bash
./build/ssta-x -l benchmark/tau2013_contest_library.v1.0.lb -d benchmark/tau2015/c7552 -b 10
```

`examples/c17.cpp` shows the equivalent C++ API:

```cpp
Timer timer;
timer.read_celllib("../benchmark/tau2013_contest_library.v1.0.lb");
timer.read_verilog("../benchmark/tau2015/c17/c17.v");
timer.read_spef("../benchmark/tau2015/c17/c17.spef");
timer.read_timing("../benchmark/tau2015/c17/c17.timing");
timer.run();
```

Use `Timer::run_timed()` instead of `run()` to get forward and backward
propagation times back as a `TimingResult` without the printed breakdown.

## Tuning the kernels

`ssta/cuda/tuning_macros.cuh` exposes per-kernel block sizes and
`__launch_bounds__` settings as overridable macros, each defaulting to the value
used in the paper. Override them at configure time:

```bash
cmake -S . -B build -DSSTA_X_TUNING_DEFS="-DBLOCK_MAX=512;-DBLOCK_DELAY=128"
```

## Repository layout

```
ssta/cell/        Liberty (.lib) parser and cell timing tables
ssta/verilog/     gate-level Verilog netlist parser
ssta/timer/       timing graph construction, levelization, GPU orchestration
  timer.cpp         levelization and GPU-efficient data structure construction
  spef.cpp          SPEF parasitics ingestion into RC tree arrays
  timing.cpp        timing assertion (.timing) parser
  cuda.cpp          host-side memory management and kernel scheduling
ssta/cuda/        the GPU kernels
  rct.cu            RC tree BFS, load capacitance, Elmore delay, slew
  timing.cu         arc interpretation, delay/slew lookup, statistical max
  tuning_macros.cuh compile-time tuning surface
ssta/unit/        unit conversion for library and parasitics values
ssta/utility/     tokenizer, logging, filesystem helpers
main/             the ssta-x command-line driver
benchmark/        TAU contest library and small designs
```

## Benchmarks

The repository bundles the TAU 2013 contest cell library and 15 small ISCAS-85
and ISCAS-89 designs (about 6 MB) so the tool is runnable immediately. The
large industrial designs used in the paper — `leon3mp`, `leon2`, `netcard`,
`vga_lcd`, `b19`, `des_perf`, `aes_core`, `ac97_ctrl` — total roughly 11 GB and
are distributed by the TAU contests rather than bundled here. See
[benchmark/README.md](benchmark/README.md).

## Third-party components

| Component | Location | License |
| --- | --- | --- |
| [Parser-SPEF](https://github.com/OpenTimer/Parser-SPEF) (with bundled PEGTL) | `3rd-party/Parser-SPEF` (submodule) | MIT |
| [nholthaus/units](https://github.com/nholthaus/units) | `ssta/unit/units.hpp` | MIT |
| [OpenTimer](https://github.com/OpenTimer/OpenTimer) — Verilog parser and exception handling, adapted | `ssta/verilog/`, `ssta/exception.hpp` | MIT |

## Citation

```bibtex
@article{chang2026sstax,
  author  = {Chang, Chih-Chun and Chung, Yi-Hua and Lee, Wan Luan and
             Zhang, Boyang and Huang, Tsung-Wei},
  title   = {{SSTA-X}: {GPU}-Accelerated First-Order Block-Based Statistical
             Static Timing Analysis},
  journal = {IEEE Transactions on Computer-Aided Design of Integrated Circuits
             and Systems},
  year    = {2026},
  doi     = {10.1109/TCAD.2026.3696812}
}
```

## License

MIT — see [LICENSE](LICENSE).
