// SSTA-X command-line driver.
//
// Runs GPU-accelerated first-order block-based statistical static timing
// analysis on a design described by a Liberty library, a Verilog netlist,
// a SPEF parasitics file, and a TAU-contest timing assertion file.

#include <ssta/timer/timer.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace {

void usage(const char* prog) {
  std::cout
    << "usage: " << prog << " -l <celllib> -v <verilog> -s <spef> -t <timing>\n"
    << "\n"
    << "required:\n"
    << "  -l, --celllib <path>   Liberty cell library (.lb / .lib)\n"
    << "  -v, --verilog <path>   gate-level Verilog netlist (.v)\n"
    << "  -s, --spef    <path>   interconnect parasitics (.spef)\n"
    << "  -t, --timing  <path>   timing assertions (.timing)\n"
    << "\n"
    << "optional:\n"
    << "  -d, --design  <dir>    read <dir>/<name>.{v,spef,timing} using the\n"
    << "                         directory name; overridden by -v/-s/-t\n"
    << "  -b, --benchmark <N>    re-run the analysis N times and report the\n"
    << "                         mean forward/backward propagation runtime\n"
    << "                         instead of the timing report\n"
    << "  -h, --help             show this message\n";
}

// Returns the argument following flag `i`, or exits if it is missing.
std::string take_value(int argc, char* argv[], int& i) {
  if (i + 1 >= argc) {
    std::cerr << "error: " << argv[i] << " requires a value\n";
    std::exit(EXIT_FAILURE);
  }
  return argv[++i];
}

}  // namespace

int main(int argc, char* argv[]) {

  std::string celllib, verilog, spef, timing, design;
  int repeat = 0;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-l" || arg == "--celllib") {
      celllib = take_value(argc, argv, i);
    }
    else if (arg == "-v" || arg == "--verilog") {
      verilog = take_value(argc, argv, i);
    }
    else if (arg == "-s" || arg == "--spef") {
      spef = take_value(argc, argv, i);
    }
    else if (arg == "-t" || arg == "--timing") {
      timing = take_value(argc, argv, i);
    }
    else if (arg == "-d" || arg == "--design") {
      design = take_value(argc, argv, i);
    }
    else if (arg == "-b" || arg == "--benchmark") {
      repeat = std::stoi(take_value(argc, argv, i));
      if (repeat <= 0) {
        std::cerr << "error: --benchmark expects a positive count\n";
        return EXIT_FAILURE;
      }
    }
    else if (arg == "-h" || arg == "--help") {
      usage(argv[0]);
      return EXIT_SUCCESS;
    }
    else {
      std::cerr << "error: unknown option '" << arg << "'\n\n";
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  // A design directory such as benchmark/tau2015/c17 holds c17.v, c17.spef and
  // c17.timing, so the three paths can be derived from the directory name.
  if (!design.empty()) {
    std::filesystem::path dir(design);
    if (dir.filename().empty()) {  // tolerate a trailing slash
      dir = dir.parent_path();
    }
    const std::string name = dir.filename().string();
    if (verilog.empty()) verilog = (dir / (name + ".v")).string();
    if (spef.empty())    spef    = (dir / (name + ".spef")).string();
    if (timing.empty())  timing  = (dir / (name + ".timing")).string();
  }

  const std::vector<std::pair<const char*, const std::string*>> required = {
    {"--celllib", &celllib}, {"--verilog", &verilog},
    {"--spef", &spef},       {"--timing", &timing},
  };
  for (const auto& [flag, value] : required) {
    if (value->empty()) {
      std::cerr << "error: missing required option " << flag << "\n\n";
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }
  for (const auto& [flag, value] : required) {
    if (!std::filesystem::exists(*value)) {
      std::cerr << "error: " << flag << " file not found: " << *value << "\n";
      return EXIT_FAILURE;
    }
  }

  if (repeat == 0) {
    Timer timer;
    timer.read_celllib(celllib);
    timer.read_verilog(verilog);
    timer.read_spef(spef);
    timer.read_timing(timing);
    timer.run();
    return EXIT_SUCCESS;
  }

  // Benchmark mode. Each iteration rebuilds the timer from scratch so that no
  // state carries over between runs; only the GPU propagation is timed.
  double forward = 0.0, backward = 0.0;
  for (int i = 0; i < repeat; ++i) {
    Timer timer;
    timer.read_celllib(celllib);
    timer.read_verilog(verilog);
    timer.read_spef(spef);
    timer.read_timing(timing);

    const TimingResult r = timer.run_timed();
    forward  += r.forward_ms;
    backward += r.backward_ms;
    std::cout << "run " << (i + 1) << "/" << repeat
              << ": forward " << r.forward_ms << " ms"
              << ", backward " << r.backward_ms << " ms" << std::endl;
  }

  std::cout << "\nmean over " << repeat << " runs:\n"
            << "  forward propagation:  " << forward / repeat << " ms\n"
            << "  backward propagation: " << backward / repeat << " ms\n"
            << "  total propagation:    " << (forward + backward) / repeat
            << " ms" << std::endl;

  return EXIT_SUCCESS;
}
