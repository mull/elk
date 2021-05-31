#pragma once

#include <string>
#include "fmt/core.h"

#include "./concepts/Debuggable.hpp"

namespace elk {
  struct Interface {
    const std::string name;
    
    std::string to_debug() const {
      return fmt::format("Interface \"{}\"", name);
    }
  };

  static_assert(elk::concepts::Debuggable<Interface>);
}

