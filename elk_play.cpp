
#include "fmt/core.h"

#include "elk/Type.hpp"
#include "elk/Monad.hpp"
#include "elk/Interface.hpp"
#include "elk/IO.hpp"
#include "elk/Module.hpp"

#include "elk/std_lib/types/Int.hpp"
#include "elk/std_lib/types/Str.hpp"
#include "elk/std_lib/types/Rec.hpp"

#include "elk/concepts/Debuggable.hpp"
#include "elk/compiler/Graph.hpp"
#include "elk/compiler/hoot.hpp"

#include<iostream>
// #include<string>
// #include<vector>
#include<cassert>

std::ostream& operator<<(std::ostream& out, elk::concepts::Debuggable auto& obj) {
  out << obj.to_debug();
  return out;
}

std::ostream& operator<<(std::ostream& out, std::shared_ptr<elk::Type>& obj) {
  out << *obj;
  return out;
}

int main() {
  using namespace elk::compiler;

  Graph graph;
  auto Int = graph.insert_type({
    .name = "Int"
  });

  auto Str = graph.insert_type({
    .name = "Str",
  });

  auto ID = graph.insert_type({
    .name = "ID",
    .of = { Int }
  });

  std::cout << "Int: " << Int << "\n";
  std::cout << "Str: " << Str << "\n";
  std::cout << "ID: " << ID << "\n";

  return 0;
}