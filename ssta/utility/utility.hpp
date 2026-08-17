#pragma once

#include <ssta/utility/lambda.hpp>
#include <ssta/utility/logger.hpp>
#include <ssta/utility/tokenizer.hpp>
#include <ssta/utility/index.hpp>
#include <ssta/utility/os.hpp>
#include <ssta/utility/scope_guard.hpp>
#include <ssta/utility/unique_guard.hpp>


// Concatnate a string
template <typename... Ts>
std::string to_string(Ts&&... ts) {
  std::ostringstream oss;
  (oss << ... << ts);
  return oss.str();
}

// Resize a container to fit a given size
template <typename T>
void resize_to_fit(size_t N, T& v) {
  auto sz = v.size();
  for(; sz < N; sz = (sz==0) ? 32 : (sz << 1));
  v.resize(sz);
}

// Resize containers to fit a given size
template <typename... T, std::enable_if_t<(sizeof...(T)>1), void>* = nullptr>
void resize_to_fit(size_t N, T&...vs) {
  (resize_to_fit(N, vs), ...);
}



