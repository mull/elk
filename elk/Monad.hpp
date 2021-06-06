#pragma once

#include <string>
#include "./concepts/Debuggable.hpp"



namespace elk {
  struct Monad {
    const std::string name;

    std::string to_debug() const { return name; }
  };

  static_assert(concepts::Debuggable<Monad>);
}
