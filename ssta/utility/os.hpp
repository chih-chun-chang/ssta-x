#pragma once


#include <vector>
#include <string>
#include <functional>
#include <unistd.h>
#include <memory>
#include <cstring>
#include <pwd.h>
#include <filesystem>


// Function: user_home
std::filesystem::path user_home();

// Function: c_args
std::unique_ptr<char*, std::function<void(char**)>> c_args(const std::vector<std::string>&);






