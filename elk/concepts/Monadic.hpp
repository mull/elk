#pragma once

namespace elk {
namespace concepts {

template<typename T>
concept Monadic = requires (T a) {
  { a }; 
};
}
}