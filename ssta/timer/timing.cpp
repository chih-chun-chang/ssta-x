#include <ssta/timer/timer.hpp>

Timer& Timer::read_timing(std::filesystem::path path) {

  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Error opening timing file\n";
    std::exit(1);
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string keyword;

    iss >> keyword;

    if (keyword == "at") {
      std::string nodename;
      float afe, afl, are, arl;
      iss >> nodename >> are >> afe >> arl >> afl;
      _at[nodename][0] = afe * (1e-12); // atFallEarly[0] 
      _at[nodename][1] = afl * (1e-12); // atFallLate[0]
      _at[nodename][2] = are * (1e-12); // atRiseEarly[0]
      _at[nodename][3] = arl * (1e-12); // atRiseLate[0]
    }
    else if (keyword == "slew") {
      std::string nodename;
      float er, ef, lr, lf;
      iss >> nodename >> er >> ef >> lr >> lf;
      _slew[nodename][0] = ef * (1e-12); // slewFallEarly[0]
      _slew[nodename][1] = lf * (1e-12); // slewFallLate[0] 
      _slew[nodename][2] = er * (1e-12); // slewRiseEarly[0]
      _slew[nodename][3] = lr * (1e-12); // slewRiseLate[0]
    }
    else if (keyword == "rat") {
      std::string nodename;
      float rafe, rafl, rare, rarl;
      iss >> nodename >> rare >> rafe >> rarl >> rafl;
      _rat[nodename][0] = rafe * (1e-12); // ratFallEarly[0]
      _rat[nodename][1] = rafl * (1e-12); // ratFallLate[0]
      _rat[nodename][2] = rare * (1e-12); // ratRiseEarly[0]
      _rat[nodename][3] = rarl * (1e-12); // ratRiseLate[0]
      //IIPNode->tData.slack_early = 1;
      //IIPNode->tData.slack_late = 1;
    }
    else if (keyword == "load") {
      // not in 2013
    }
    else if (keyword == "clock") {
      std::string nodename;
      float period;
      iss >> nodename >> period;
      _clockNode = nodename;
      _clockTP = period * (1e-12);
    }
    else {
      std::cerr << "Unknown keyword: " << keyword << '\n';
    }
  }
  file.close();

  return *this;
}

