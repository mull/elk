#pragma once
#include <string>
#include <concepts>

#include "../utils/meta.hpp"

namespace elk {
namespace concepts {
  template<typename T>
  concept Named = requires (T t) {
    { t.name } -> convertible_to<int>;
  };
}
}