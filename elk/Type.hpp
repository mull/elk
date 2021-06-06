#pragma once

#include "fmt/core.h"
#include "./concepts/Debuggable.hpp"
#include <optional>
#include <memory>

namespace {
  enum class TypeCategory {
    Abstract,
    Concrete
  };
}

namespace elk {
  using namespace std;

  struct Type {
    const size_t                      id;
    const string                      name;
    const optional<shared_ptr<Type>>  of;

    auto category() const {
      return TypeCategory::Concrete;
    }

    auto to_debug() const {
      const auto type = category() == TypeCategory::Concrete 
        ? fmt::format("-> {}", name) 
        : "-> ()";
      const auto id = of 
        ? fmt::format("{}<{}>", of.value()->name, name)
        : name;

      return fmt::format("{} {}", id, type);
    }
  };


  static_assert(concepts::Debuggable<Type>);
}