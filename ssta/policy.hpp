#pragma once

// Function: log_file
inline std::string log_file() {
  if(auto str = std::getenv("OT_LOG_FILE")) {
    return str;
  }
  else return "";
}


