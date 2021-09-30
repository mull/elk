#include "utils/Status.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <any>
#include <variant>
#include <string_view>
#include <vector>
#include <memory>

#include <fmt/core.h>
#include <fmt/format.h>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

namespace def {
  struct TypeID {
    const size_t id;
    bool operator==(const TypeID& other) const { 
      return id == other.id;
    }
  };
  struct ValueID {
    const size_t id;
    bool operator==(const ValueID& other) const { 
      return id == other.id;
    }
  };
  struct BindingID {
    const size_t id;
    bool operator==(const BindingID& other) const { 
      return id == other.id;
    }
  };

  struct hash_id {
    std::size_t operator() (const TypeID &me) const { return me.id; }
    std::size_t operator() (const ValueID &type) const { return type.id; }
    std::size_t operator() (const BindingID &type) const { return type.id; }
  };

  struct Value; struct Type;
  struct ArgumentList; struct Function;
  struct Binding; struct Scope; struct Construct;

  using AstNode = std::variant<Value, Type, ArgumentList, Construct, Function, Binding, Scope>;

  struct Scope : public std::vector<AstNode> {};

  struct ArgumentList : public std::vector<Value> {};
  struct ParameterList : public std::vector<size_t> {};

  struct Function {
    const ParameterList parameters {};
    const size_t        return_type_id;
    const Scope         scope {};
  };

  struct Value; struct Type;
  using value_t = std::variant<std::nullopt_t, int64_t, char, Function>;

  struct Value {
    explicit Value(TypeID type_id, int64_t i) : type_id(type_id), v(i) {};
    explicit Value(TypeID type_id, char c) : type_id(type_id), v(c) {};
    explicit Value(TypeID type_id, Function f) : type_id(type_id), v(f) {};

    const TypeID type_id;
    const value_t v {0};
  };

  struct Constructor {
    const bool is_abstract { false };
    // TODO: This ought to fail somehow without exceptions
    const std::function<Value(TypeID)>           make         = [](auto type_id) -> Value { throw std::runtime_error("Can't construct_0 an abstract type"); };
    const std::function<Value(TypeID, std::any)> make_1       = [](auto type_id, auto) -> Value { throw std::runtime_error("Can't construct_1 an abstract type"); };
  };

  static Constructor make_int64_constructor() { 
    return Constructor {
      .is_abstract    = false,
      .make           = [](TypeID type_id) { return Value(type_id, int64_t(0)); },
      .make_1         = [](TypeID type_id, std::any value) { return Value(type_id, std::any_cast<int64_t>(value)); }
    };
  }

  static Constructor make_char_constructor() { 
    return {
      .is_abstract  = false,
      .make         = [](TypeID type_id) { return Value(type_id, char{'C'}); },
      .make_1       = [](TypeID type_id, std::any value) { return Value(type_id, std::any_cast<char>(value)); }
    };
  }

  struct Construct {
    const size_t type_id;
    const ArgumentList arguments;
  };

  struct Type {
    const std::string name;
    const def::Constructor constructor { .is_abstract = true };
  };

  struct Binding {
    const std::string name;
    const ValueID value_id;
  };
};


template <> struct fmt::formatter<def::value_t>: formatter<std::string> {
  // parse is inherited from formatter<std::string>

  template <typename FormatContext>
  auto format(const def::value_t& value, FormatContext& ctx) {
    std::string v = std::visit(overloaded {
      [](int64_t i) { return fmt::to_string(i); },
      [](char c) { return fmt::to_string(c); },
      [](const def::Function& f) {
        std::string s = "(";
        for (const auto id : f.parameters) {
          s.append(std::to_string(id));
          s.append(", ");
        }
        if (!f.parameters.empty()) s.erase(s.end() - 2, s.end());

        s.append(")");
        return s;
      },
      [](auto) { return fmt::to_string("???"); }
    }, value);

    return formatter<std::string>::format(v, ctx);
  }
};

static const auto Unit = def::Type { .name = "Unit" };
static const auto Int = def::Type { .name="Int", .constructor=def::make_int64_constructor() };
static const auto Char = def::Type { .name="Char", .constructor=def::make_char_constructor() };
static const auto Function = def::Type { .name="Function" };

static const def::TypeID  UnitID = {.id=0},
                          IntID = {.id=1},
                          CharID = {.id=2},
                          FunctionID = {.id=3};

static const size_t       STARTING_TYPE_ID = FunctionID.id + 1;


using namespace elk::utils;

struct VM {
  using VMValue = std::shared_ptr<def::Value>;

  std::unordered_map<def::TypeID, def::Type, def::hash_id> types {
    std::make_pair(UnitID, Unit),
    std::make_pair(IntID, Int),
    std::make_pair(CharID, Char),
    std::make_pair(FunctionID, Function)
  };

  std::unordered_map<def::BindingID, def::Binding, def::hash_id> bindings {};
  std::unordered_map<def::ValueID, VMValue, def::hash_id> values {};

  def::TypeID register_type(const def::Type type) {
    const auto id = def::TypeID {.id = _type_id++ };
    types.insert(std::make_pair(id, type));
    return id;
  }

  def::BindingID register_binding(const def::Binding binding) {
    size_t id = std::hash<std::string>{}(binding.name);
    const auto binding_id = def::BindingID {.id=id};
    bindings.insert(std::make_pair(binding_id, binding));
    return binding_id;
  }

  // size_t register_function(const def::Function function) {
  //   size_t id = _value_id++;
  //   const auto value = std::make_shared<def::Value>(FunctionID, function);
  //   values.insert(std::make_pair(id, value));
  //   return id;
  // }

  def::ValueID construct(const def::TypeID type_id) {
    const auto type = types.at(type_id);
    if (type.constructor.is_abstract) { throw std::runtime_error("Tried to construct an abstract type"); }

    const def::ValueID id = {.id=_value_id++};
    values.insert(std::make_pair(id, std::make_shared<def::Value>(type.constructor.make(type_id))));
    return id;
  }

  def::ValueID construct(const def::TypeID type_id, const std::any value) {
    const auto type = types.at(type_id);
    if (type.constructor.is_abstract) { throw std::runtime_error("Tried to construct an abstract type"); }

    const def::ValueID id = {.id=_value_id++};
    values.insert(std::make_pair(id, std::make_shared<def::Value>(type.constructor.make_1(type_id, value))));
    return id;
  }


  // auto interpret(const def::AstNode node) {
  //   if (std::holds_alternative<def::Construct>(node)) {
  //     const auto n_construct = std::get<def::Construct>(node);
  //     const std::any v = std::get<int64_t>(n_construct.arguments[0].v);
  //     return this->construct(n_construct.type_id, v);
  //   }


  //   throw std::runtime_error("Do not know how to interpret this node");
  // }

  // auto invoke(size_t value_id, def::ArgumentList arguments = {}) {
  //   const auto value = value_at(value_id).get();
  //   if (value->type_id != FunctionID) {
  //     throw std::runtime_error(fmt::format("Value#{} is not the same type ({}) as Function ({})", value_id, value->type_id, FunctionID));
  //   }

  //   const auto fun = std::get<def::Function>(value->v);
  //   for (const def::AstNode& node : fun.scope) {
  //     return this->interpret(node);
  //   }

  //   return this->construct(fun.return_type_id);
  // }

  void debug(def::TypeID type_id) {
    const auto type = types.at(type_id);
    fmt::print("debug:type        ({}, {})\n", type.name, type.constructor.is_abstract ? "Abstract" : "Concrete");
  }

  void debug(const def::ValueID value_id) {
    const auto value = values.at(value_id);
    const auto type = types.at(value->type_id);
    fmt::print("debug:val         ({}, {}, [use_count:{}])\n", type.name, value->v, value.use_count() - 1);
  }

  void debug(const def::BindingID binding_id) {
    const auto binding = bindings.at(binding_id);
    fmt::print("debug:binding     [name:{} value_id:{}]\n", binding.name, binding.value_id.id);
    const auto val = values.at(binding.value_id);
    fmt::print("                  ");
    debug(val->type_id);
    fmt::print("                  ");
    debug(binding.value_id);
  }

  private:
  size_t _type_id {STARTING_TYPE_ID};
  size_t _value_id {0};
};



int main() {
  VM vm;
  fmt::print("---- Types\n");
  vm.debug(UnitID);
  // vm.construct(UnitID);

  vm.debug(IntID);
  const auto int_value_id = vm.construct(IntID);
  vm.debug(int_value_id);

  vm.debug(CharID);
  const auto char_value_id = vm.construct(CharID);
  vm.debug(char_value_id);

  fmt::print("----\n");
  fmt::print("---- Bindings\n");
  fmt::print("---- ---- Default constructed\n");
  const auto binding_x = vm.register_binding(def::Binding {
    .name = "x",
    .value_id = int_value_id
  });
  vm.debug(binding_x);
  const auto binding_y = vm.register_binding(def::Binding {
    .name = "y",
    .value_id = char_value_id 
  });
  vm.debug(binding_y);

  fmt::print("---- ---- Initialised with value\n");
  const auto value_id_1 = vm.construct(IntID, int64_t{4});
  const auto binding_initialised_int = vm.register_binding(def::Binding {
    .name = "initialised_int",
    .value_id = value_id_1 
  });
  vm.debug(binding_initialised_int);

  const auto value_id_a = vm.construct(CharID, char{'a'});
  const auto binding_initialised_char = vm.register_binding(def::Binding {
    .name = "initialised_char",
    .value_id = value_id_a
  });
  vm.debug(binding_initialised_char);


  // fmt::print("----\n");
  // fmt::print("---- Functions\n");
  // fmt::print("---- ---- With empty scope\n");
  // fmt::print("---- ---- ---- With arguments\n");
  // const auto function_id_1 = vm.register_function(def::Function {
  //   .parameters = {{IntID, IntID}},
  //   .return_type_id = IntID
  // });
  // vm.debug(vm.value_at(function_id_1));


  // const auto function_binding_id_1 = vm.register_binding(def::Binding {
  //   .name = "function_binding_1",
  //   .value_id = function_id_1,
  // });
  // vm.debug(vm.binding_at(function_binding_id_1));


  // fmt::print("\n");
  // fmt::print("---- ---- With non-empty scope\n");
  // fmt::print("---- ---- ---- Construct IntID 1024\n");
  // const auto function_id_2 = vm.register_function(def::Function {
  //   .parameters = {},
  //   .return_type_id = IntID,
  //   .scope = def::Scope {{
  //     def::Construct {
  //       .type_id = IntID,
  //       .arguments {{
  //         def::Value(IntID, int64_t{1024})
  //       }}          
  //     }
  //   }}
  // });
  // vm.debug(vm.value_at(function_id_2));
  // const auto return_value_id_2 = vm.invoke(function_id_2);
  // vm.debug(vm.value_at(return_value_id_2));

  // fmt::print("\n");
  // fmt::print("---- ---- With parameters\n");
  // const auto function_id_3 = vm.register_function(def::Function {
  //   .parameters = {{IntID}},
  //   .return_type_id = IntID,
  //   .scope = def::Scope {{
  //     def::Construct {
  //       .type_id = IntID,
  //       .arguments {{
  //         def::Value(IntID, int64_t{1024})
  //       }}          
  //     }
  //   }}
  // });
  return 0;
}