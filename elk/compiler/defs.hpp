#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../Type.hpp"

namespace elk {
namespace compiler {
  using namespace std;
  struct FuncDef;

  struct TypeDef {
    const string                      name;
    const optional<shared_ptr<Type>>  of;
    const optional<vector<FuncDef>>   constructors;
  };

  struct Argument {
    const string            name;
    const shared_ptr<Type>  type;
  };

  struct FuncDef {
    const string            name;
    const vector<Argument>  arg;
    const shared_ptr<Type>  ret;
  };
}
}