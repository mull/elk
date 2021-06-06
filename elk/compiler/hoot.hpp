#pragma once

/**
 * The main goal of this module is to enable type driven (abstract) development.
 */

#include <string>
#include <optional>
#include <memory>

#include "./Graph.hpp"

namespace elk {
namespace compiler {
namespace hoot {
  using namespace std;



  bool
  conversion_paths(const Graph& g, shared_ptr<Type> from, shared_ptr<Type> to) {
    // id
    if (from == to) {
      return true;
    }

    // subtype to parent
    if (from->of && from->of.value() == to) return true;

    // parent to subtype
    if (to->of && to->of.value() == from) return true;

    // siblings
    if (to->of && from->of && to->of.value() == from->of.value()) {
      return conversion_paths(g, from, from->of.value()) && 
             conversion_paths(g, from->of.value(), to);
    };

    throw std::runtime_error("case not handled");
  }
}
}
}