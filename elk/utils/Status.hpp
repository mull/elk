#pragma once

#include <optional>

namespace elk {
namespace utils {
  enum class Error {
    TOO_FEW_ARGUMENTS
  };

  template<typename T>
  struct Status {
    std::optional<T> result;
    std::optional<Error> error;
  };
}
}