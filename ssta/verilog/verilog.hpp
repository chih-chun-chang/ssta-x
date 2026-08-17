#pragma once

// https://github.com/OpenTimer/OpenTimer/blob/master/ot/verilog/verilog.hpp

#include <ssta/headerdef.hpp>
#include <ssta/static/logger.hpp>
#include <ssta/utility/utility.hpp>
#include <ssta/exception.hpp>

namespace vlog {

// Struct: Gate
struct Gate 
{
  std::string name;
  std::string cell;
  std::unordered_map<std::string, std::string> cellpin2net;
  std::unordered_map<std::string, std::string> net2cellpin;
};

std::ostream& operator << (std::ostream&, const Gate&);

// ------------------------------------------------------------------------------------------------

// Struct: Module
struct Module {
  std::string name;
  std::vector<std::string> ports;
  std::vector<std::string> wires;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<Gate> gates;
  std::string info() const;
};

std::ostream& operator << (std::ostream&, const Module&);

// ------------------------------------------------------------------------------------------------

Module read_verilog(const std::filesystem::path&);

};  // end of namespace ------- --------------------------------------------------------------------





