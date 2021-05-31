#pragma once

#include "fmt/core.h"

#include "./concepts/Debuggable.hpp"

namespace {
  enum class TypeCategory {
    Abstract,
    Concrete
  };
}

namespace elk {
  struct Type {
    const std::string name;
    const TypeCategory category;

    auto to_debug() const {
      return fmt::format("{} ({})", name, (category == TypeCategory::Concrete) ? "concrete" : "abstract");
    }
  };


  static_assert(elk::concepts::Debuggable<Type>);
}