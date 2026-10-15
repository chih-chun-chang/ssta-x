// Runs SSTA-X on the bundled c17 design through the C++ API.
// Built by default (SSTA_X_BUILD_EXAMPLES); run it from the build directory:
//   cd build && ./examples/c17
#include <ssta/timer/timer.hpp>

int main() {
  Timer timer;
  timer.read_celllib("../benchmark/tau2013_contest_library.v1.0.lb");
  timer.read_verilog("../benchmark/tau2015/c17/c17.v");
  timer.read_spef("../benchmark/tau2015/c17/c17.spef");
  timer.read_timing("../benchmark/tau2015/c17/c17.timing");
  timer.run();
  return 0;
}
