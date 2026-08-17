#pragma once

#include <array>
#include <ssta/utility/utility.hpp>

namespace cell {

enum class TimingSense {
  PositiveUnate,
  NegativeUnate,
  NonUnate
};

inline const char* to_string(TimingSense sense) {
  switch (sense) {
    case TimingSense::PositiveUnate: return "positive_unate";
    case TimingSense::NegativeUnate: return "negative_unate";
    case TimingSense::NonUnate: return "non_unate";
    default: return "unknown";
  }
}

enum class EdgeType {
  Falling,
  Rising
};

struct Timing_Data {
  bool hasData = false;
  TimingSense sense;
  std::array<float, 9> fallSlew = {};
  std::array<float, 9> riseSlew = {};
  std::array<float, 9> fallDelay = {};
  std::array<float, 9> riseDelay = {};
};

struct Wire_Timing_Data {
  std::array<float, 2> fallDelay = {};
  std::array<float, 2> riseDelay = {};
  std::array<float, 2> fallSlew = {};
  std::array<float, 2> riseSlew = {};
};

struct Constraint_Data {
  bool hasData;
  EdgeType edgeType;
  std::array<float, 3> fall;
  std::array<float, 3> rise;
};

struct Cell
{
  std::string name;
  int numiPins;  
  int numoPins;
  bool hasClkPin;
  std::string clkPinName;
  //float clkPinFallCap;
  //float clkPinRiseCap;
  std::vector<std::string> iPinNames;
  std::vector<std::string> oPinNames;
  //std::vector<float> iPinFallCap;
  //std::vector<float> iPinRiseCap;
  // make these to a hash table
  std::unordered_map<std::string, float> iPinFallCap;
  std::unordered_map<std::string, float> iPinRiseCap;
  std::unordered_map<std::string, std::unordered_map<std::string, Timing_Data>> ioTiming;
  std::unordered_map<std::string, Timing_Data> coTiming;
  std::unordered_map<std::string, Constraint_Data> setup;
  std::unordered_map<std::string, Constraint_Data> hold;
};

struct Celllib
{
  float metal0ResScaleFactor;
  float metal0CapScaleFactor;
  float metal3ResScaleFactor;
  float metal3CapScaleFactor;
  std::unordered_map<std::string, std::unique_ptr<Cell>> lib;
};

Celllib read_celllib(const std::filesystem::path& path);
TimingSense parseTimingSense(const std::string& senseStr);
EdgeType parseEdgeType(const std::string& edgeTypeStr);
template <std::size_t N>
void parseFloats(std::array<float, N>& arr, std::istringstream& stream);


};  // end of namespace  -----------------------------------------------------------------------
