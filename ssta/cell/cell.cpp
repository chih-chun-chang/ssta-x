#include <ssta/cell/cell.hpp>

namespace cell {


// Procedure: read_celllib
Celllib read_celllib(const std::filesystem::path& path) {

  Celllib celllib;
  
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error(path);

  std::string currentCellName;
  std::string line;
  std::string keyword;
  std::istringstream iss;

  while (std::getline(file, line)) { // TODO: buffer
    iss.clear();
    iss.str(line);
    iss >> keyword;
    if (keyword == "metal") {
      int sigmaCorner;
      float resistanceScaleFactor, capacitanceScaleFactor;
      iss >> sigmaCorner >> resistanceScaleFactor >> capacitanceScaleFactor;
      if (sigmaCorner == 0) {
        celllib.metal0ResScaleFactor = resistanceScaleFactor;
        celllib.metal0CapScaleFactor = capacitanceScaleFactor;
      }
      if (sigmaCorner == 3) {
        celllib.metal3ResScaleFactor = resistanceScaleFactor;
        celllib.metal3CapScaleFactor = capacitanceScaleFactor;
      }
    }
    else if (keyword == "cell") {
      iss >> currentCellName;
      if (celllib.lib.find(currentCellName) != celllib.lib.end())
        std::runtime_error("duplicate cell reading error\n");
      auto cell = std::make_unique<Cell>();
      cell->name = currentCellName;
      cell->hasClkPin = 0;
      cell->numiPins = 0;
      cell->numoPins = 0;
      celllib.lib[currentCellName] = std::move(cell);
    }
    else if (keyword == "pin") {
      if (celllib.lib.find(currentCellName) == celllib.lib.end())
        std::runtime_error("celllib reading error: pin\n");
      std::string pinName, pinTypeStr;
      iss >> pinName >> pinTypeStr;
      if (pinTypeStr == "input") {
        celllib.lib[currentCellName]->numiPins++;
        celllib.lib[currentCellName]->iPinNames.emplace_back(pinName);
        float fallCap, riseCap;
        iss >> fallCap >> riseCap;
        //celllib.lib[currentCellName]->iPinFallCap.emplace_back(fallCap);
        //celllib.lib[currentCellName]->iPinRiseCap.emplace_back(riseCap);
        celllib.lib[currentCellName]->iPinFallCap[pinName] = fallCap;
        celllib.lib[currentCellName]->iPinRiseCap[pinName] = riseCap;
      }
      else if (pinTypeStr == "clock") {
        celllib.lib[currentCellName]->hasClkPin = 1;
        celllib.lib[currentCellName]->clkPinName = pinName;
        float fallCap, riseCap;
        iss >> fallCap >> riseCap;
        //celllib.lib[currentCellName]->clkPinFallCap = fallCap;
        //celllib.lib[currentCellName]->clkPinRiseCap = riseCap;
        celllib.lib[currentCellName]->iPinFallCap[pinName] = fallCap;
        celllib.lib[currentCellName]->iPinRiseCap[pinName] = riseCap;
      }
      else {
        celllib.lib[currentCellName]->numoPins++;
        celllib.lib[currentCellName]->oPinNames.emplace_back(pinName);
      }

    }
    else if (keyword == "timing") {
      if (celllib.lib.find(currentCellName) == celllib.lib.end())
        std::runtime_error("celllib reading error: pin\n");

      std::string iName, oName, senseStr;
      iss >> iName >> oName >> senseStr;
      bool iscoTiming = 0;

      auto ipIt = std::find(
        celllib.lib[currentCellName]->iPinNames.begin(), 
        celllib.lib[currentCellName]->iPinNames.end(), 
        iName
      );
      
      if (ipIt == celllib.lib[currentCellName]->iPinNames.end()) {
        if (iName == celllib.lib[currentCellName]->clkPinName)
          iscoTiming = 1;
        else
          throw std::runtime_error("Invalid input/clk pin: " + iName);
      }

      auto opIt = std::find(
        celllib.lib[currentCellName]->oPinNames.begin(), 
        celllib.lib[currentCellName]->oPinNames.end(), 
        oName
      );

      if (opIt == celllib.lib[currentCellName]->oPinNames.end())
        throw std::runtime_error(currentCellName + ": Invalid output pin: " + oName);

      if (celllib.lib[currentCellName]->ioTiming[iName].find(oName) != celllib.lib[currentCellName]->ioTiming[iName].end()) {
        throw std::runtime_error(currentCellName + ": ioTiming duplicate input and output: " + iName + " " + oName);
      }
      celllib.lib[currentCellName]->ioTiming[iName][oName] = Timing_Data{};

      // Parse the floating-point numbers
      std::string remainingLine = line.substr(iss.tellg());
      std::istringstream dataStream(remainingLine);
      if (iscoTiming == 0) {
        celllib.lib[currentCellName]->ioTiming[iName][oName].hasData = 1;
        celllib.lib[currentCellName]->ioTiming[iName][oName].sense = parseTimingSense(senseStr); 
        parseFloats(celllib.lib[currentCellName]->ioTiming[iName][oName].fallSlew, dataStream);
        parseFloats(celllib.lib[currentCellName]->ioTiming[iName][oName].riseSlew, dataStream);
        parseFloats(celllib.lib[currentCellName]->ioTiming[iName][oName].fallDelay, dataStream);
        parseFloats(celllib.lib[currentCellName]->ioTiming[iName][oName].riseDelay, dataStream);
      }
      else {
        celllib.lib[currentCellName]->coTiming[oName].hasData = 1;
        celllib.lib[currentCellName]->coTiming[oName].sense = parseTimingSense(senseStr);      
        parseFloats(celllib.lib[currentCellName]->coTiming[oName].fallSlew, dataStream);
        parseFloats(celllib.lib[currentCellName]->coTiming[oName].riseSlew, dataStream);
        parseFloats(celllib.lib[currentCellName]->coTiming[oName].fallDelay, dataStream);
        parseFloats(celllib.lib[currentCellName]->coTiming[oName].riseDelay, dataStream);
      }
    }
    else if (keyword == "setup") {
      if (celllib.lib.find(currentCellName) == celllib.lib.end())
        std::runtime_error("celllib reading error: pin\n");
      
      std::string cName, iName, edgeTypeStr;
      iss >> cName >> iName >> edgeTypeStr;
      
      if (cName != celllib.lib[currentCellName]->clkPinName)
        throw std::runtime_error(currentCellName + ": Invalid clk pin: " + cName);
      
      auto ipIt = std::find(
        celllib.lib[currentCellName]->iPinNames.begin(), 
        celllib.lib[currentCellName]->iPinNames.end(), 
        iName
      );
      
      if (ipIt == celllib.lib[currentCellName]->iPinNames.end())
        throw std::runtime_error(currentCellName + ": Invalid input pin: " + iName);
        
      // Parse the floating-point numbers
      std::string remainingLine = line.substr(iss.tellg());
      std::istringstream dataStream(remainingLine);
      if (celllib.lib[currentCellName]->setup.find(iName) != celllib.lib[currentCellName]->setup.end())
        throw std::runtime_error(currentCellName + ": Duplicate ipin name (setup): " + iName);
      celllib.lib[currentCellName]->setup[iName] = Constraint_Data{};
      celllib.lib[currentCellName]->setup[iName].hasData = 1;
      celllib.lib[currentCellName]->setup[iName].edgeType = parseEdgeType(edgeTypeStr); 
      parseFloats(celllib.lib[currentCellName]->setup[iName].fall, dataStream);
      parseFloats(celllib.lib[currentCellName]->setup[iName].rise, dataStream);
    }
    else if (keyword == "hold") {
      if (celllib.lib.find(currentCellName) == celllib.lib.end())
        std::runtime_error("celllib reading error: pin\n");

      std::string cName, iName, edgeTypeStr;
      iss >> cName >> iName >> edgeTypeStr;
      if (cName != celllib.lib[currentCellName]->clkPinName)
        throw std::runtime_error("Invalid clk pin: " + cName);
      
      auto ipIt = std::find(
        celllib.lib[currentCellName]->iPinNames.begin(), 
        celllib.lib[currentCellName]->iPinNames.end(), 
        iName
      );
      
      if (ipIt == celllib.lib[currentCellName]->iPinNames.end())
        throw std::runtime_error("Invalid input pin: " + iName);
        
      // Parse the floating-point numbers
      std::string remainingLine = line.substr(iss.tellg());
      std::istringstream dataStream(remainingLine);
      if (celllib.lib[currentCellName]->hold.find(iName) != celllib.lib[currentCellName]->hold.end())
        throw std::runtime_error(currentCellName + ": Duplicate ipin name (hold): " + iName);
      celllib.lib[currentCellName]->hold[iName] = Constraint_Data{};
      celllib.lib[currentCellName]->hold[iName].hasData = 1;
      celllib.lib[currentCellName]->hold[iName].edgeType = parseEdgeType(edgeTypeStr); 
      parseFloats(celllib.lib[currentCellName]->hold[iName].fall, dataStream);
      parseFloats(celllib.lib[currentCellName]->hold[iName].rise, dataStream);
    }
    else {
      std::runtime_error("Unknown keyword: " + keyword);
    }
  } // end of while
  
  file.close();
  return celllib;
}

TimingSense parseTimingSense(const std::string& senseStr)
{
  if (senseStr == "positive_unate") return TimingSense::PositiveUnate;
  if (senseStr == "negative_unate") return TimingSense::NegativeUnate;
  if (senseStr == "non_unate") return TimingSense::NonUnate;
  throw std::runtime_error("Invalid timing sense: " + senseStr);
}

EdgeType parseEdgeType(const std::string& edgeTypeStr) 
{
  if (edgeTypeStr == "falling") return EdgeType::Falling;
  if (edgeTypeStr == "rising") return EdgeType::Rising;
  throw std::runtime_error("Invalid edge type: " + edgeTypeStr);
}

template <std::size_t N>
void parseFloats(std::array<float, N>& arr, std::istringstream& stream)
{
  std::size_t count = 0;
  float value;
  while (count < arr.size() && stream >> value) 
  {
    arr[count++] = value;
  }
}


};  // end of namespace  -----------------------------------------------------------------------
