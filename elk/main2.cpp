#include <string>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include <cassert>
#include <list>
#include <algorithm>
#include <deque>

#include <fmt/core.h>
#include <fmt/format.h>

namespace vm {
  struct TypeID {
    const size_t id;
    bool operator==(const TypeID& other) const { return id == other.id; }
  };
  struct ValueID {
    const size_t id;
    bool operator==(const ValueID& other) const { return id == other.id; }
  };

  struct Type {
    const TypeID id;
    const std::string name;
  };

  struct Value {
    const TypeID type_id;
    const ValueID value_id;
  };
}

namespace builder {
  using Symbol = std::string;

  struct TypeRef { const Symbol symbol; };
  struct BindingRef { const Symbol symbol; };
  
  struct ScalarValue {
    using val_t = std::variant<int64_t>;

    const TypeRef type;
    const val_t value;
  };

  using Expression = std::variant<TypeRef, BindingRef, ScalarValue>;

  struct MakeBinding {
    const Symbol symbol;
    const Expression value;
  };

  using Statement = std::variant<MakeBinding, Expression>;

  using FunctionParam = std::pair<Symbol, TypeRef>;
  using FunctionParams = std::list<FunctionParam>;
  using FunctionBody = std::list<Statement>;
  struct Function {
    const FunctionParams parameters;
    const FunctionBody body;
  };


  TypeRef type(std::string name) { return TypeRef { .symbol = Symbol(name) }; }
  ScalarValue scalar(std::string type_name, ScalarValue::val_t val) { return ScalarValue { .type = type(type_name), .value = val }; }
  BindingRef binding(std::string name) { return BindingRef { .symbol = Symbol(name) }; }

  FunctionParam make_param(Symbol name, TypeRef type) {
    return std::make_pair(name, type);
  }

  FunctionParams make_function_params(std::initializer_list<FunctionParam> params) {
    return FunctionParams(params);
  }
  
  template<typename... Args>
  auto make_function_body(Args&&... args) {
    std::list<Statement> statements {};
    (statements.push_back(std::forward<Args>(args)), ...);
    return statements;
  }


  auto make_function(FunctionParams&& params, FunctionBody&& body) {
    return Function {
      .parameters = std::move(params),
      .body = std::move(body)
    };
  }

  MakeBinding make_binding(std::string name, Expression value) {
    assert(!name.empty());

    return MakeBinding {
      .symbol = Symbol(name),
      .value = value
    };
  };
};


struct hash_id {
  std::size_t operator() (const vm::TypeID &me) const { return me.id; }
  std::size_t operator() (const vm::ValueID &type) const { return type.id; }
};

struct Runtime;
class Scope {
  public:
  std::optional<vm::Type> find_type_by_name(const std::string& name) {
    const auto id = type_names.at(name);
    return types.at(id);
  }

  friend class Runtime;

  private:
  std::unordered_map<vm::TypeID, vm::Type, hash_id> types {};
  std::unordered_map<std::string, vm::TypeID> type_names {};
  std::unordered_map<vm::ValueID, builder::Function, hash_id> functions;
};


class Runtime {
  public:

  Runtime() {
    Scope s;
    auto id = vm::TypeID {.id = 1 };
    s.types.insert(std::make_pair(id, vm::Type { .id = id, .name="Int" }));
    s.type_names.insert(std::make_pair("Int", id));
    scopes.push_back(s);
    cur_scope = std::rbegin(scopes);
  }

  void eval(builder::Statement&&) {}
  void eval(builder::Expression&&) {}
  void eval(builder::Function&& fun) {
    // 1. validate type refs in fun.parameters
    for (auto param : fun.parameters) {
      auto cur = cur_scope;
      auto found = false;
      while (cur != std::rend(scopes)) {
        if (cur->type_names.contains(param.second.symbol)) {
          found = true; break;
        }
        cur = std::next(cur);
      }
      if (!found) {
        throw std::runtime_error("Type not found");
      }

      auto type = cur->types.at(cur->type_names.at(param.second.symbol));
      fmt::print("Found type {}\n", type.name);
    }
    // 2. store function as a value

    // 3. return value_id ?
  }

  private:
  std::list<Scope> scopes;
  std::list<Scope>::reverse_iterator cur_scope;
};

int main() {
  using namespace builder;

  Runtime vm;

  vm.eval(
    make_binding("x", scalar("Int", 42))
  );

   vm.eval(
     make_function(
       make_function_params({
         make_param("x", type("Int")),
         make_param("y", type("Int"))
      }),
      make_function_body(
        binding("x") 
      )
    )
  );
}