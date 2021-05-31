#include<iostream>
#include<string>

#include "fmt/core.h"

#include "elk/Type.hpp"
#include "elk/Monad.hpp"
#include "elk/Interface.hpp"
// #include "elk/IO.hpp"
// #include "elk/Module.hpp"

#include "elk/std_lib/types/Int.hpp"
#include "elk/std_lib/types/Str.hpp"

#include "elk/concepts/Debuggable.hpp"

std::ostream& operator<<(std::ostream& out, elk::concepts::Debuggable auto& obj) {
  out << obj.to_debug();
  return out;
}

int main() {
  return 0;
}