#pragma once
#include <string>
#include <concepts>

#include "../utils/meta.hpp"
#include "./Named.hpp"

namespace {
  template<typename T>
  concept HasDebugFunction = requires (T a) {
    { a.to_debug() } -> convertible_to<std::string>;
  };
}

namespace elk {
namespace concepts {
  template<typename T>
  concept Debuggable = HasDebugFunction<T> || Named<T>;
  }
}