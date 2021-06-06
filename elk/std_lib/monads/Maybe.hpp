#pragma once

#include "elk/concepts/Monadic.hpp"
#include "elk/concepts/Debuggable.hpp"
#include "elk/concepts/Named.hpp"

#include "elk/Type.hpp"

#include <optional>
#include <memory>

namespace elk {
namespace std_lib {
namespace monads {
  using namespace std;

  struct Maybe {
    using Object = shared_ptr<Type>;
    Object of;

    Maybe(Object of) : of(of) {}

  };

  static_assert(concepts::Monadic<Maybe>);
  // static_assert(concepts::Debuggable<Maybe>);
  // static_assert(concepts::Named<Maybe>);
}
}
} 