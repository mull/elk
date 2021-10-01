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

  using ID = std::variant<TypeID, ValueID>;

  struct hash_id {
    std::size_t operator() (const TypeID &me) const { return me.id; }
    std::size_t operator() (const ValueID &type) const { return type.id; }
  };

  struct Value; struct Type;
  struct ArgumentList; struct Function;
  struct Construct;

  struct ParameterList : public std::vector<TypeID> {};

  struct Function {
    const ParameterList parameters {};
    const TypeID        return_type_id;
    // const Scope         scope {};
  };

  struct Value; struct Type;
  using value_t = std::variant<std::nullopt_t, int64_t, char>;

  struct Value {
    explicit Value(TypeID type_id, int64_t i) : type_id(type_id), v(i) {};
    explicit Value(TypeID type_id, char c) : type_id(type_id), v(c) {};

    const TypeID type_id;
    const value_t v {0};
  };

  struct Constructor {
    const bool is_abstract { false };
    // TODO: This ought to fail somehow without exceptions
    const std::function<Value(TypeID)>           make   = [](auto type_id) -> Value { throw std::runtime_error("Can't construct_0 an abstract type"); };
    const std::function<Value(TypeID, std::any)> make_1 = [](auto type_id, auto) -> Value { throw std::runtime_error("Can't construct_1 an abstract type"); };
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

  struct ArgumentList : public std::vector<ValueID> {};
  struct Construct {
    const TypeID type_id;
    const ArgumentList arguments;
  };

  struct Type {
    const std::string name;
    const def::Constructor constructor { .is_abstract = true };
  };
};

std::any unpack_value_t(def::value_t t) {
  if (std::holds_alternative<char>(t)) return std::get<char>(t);
  if (std::holds_alternative<int64_t>(t)) return std::get<int64_t>(t);
  return std::nullopt;
}

template <> struct fmt::formatter<def::value_t>: formatter<std::string> {
  // parse is inherited from formatter<std::string>

  template <typename FormatContext>
  auto format(const def::value_t& value, FormatContext& ctx) {
    std::string v = std::visit(overloaded {
      [](int64_t i) { return fmt::to_string(i); },
      [](char c) { return fmt::to_string(c); },
      [](const def::Function& f) {
        std::string s = "(";
        for (const auto type_id : f.parameters) {
          s.append(std::to_string(type_id.id));
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



struct VM {
  using VMValue = std::shared_ptr<def::Value>;

  std::unordered_map<def::TypeID, def::Type, def::hash_id>  types {};
  std::unordered_map<std::string, def::TypeID>              type_names {};
  std::unordered_map<def::ValueID, VMValue, def::hash_id>   values {};
  std::optional<const VM*>                                  parent;

  VM() = default;

  def::TypeID register_type(const def::Type type) {
    const auto id = def::TypeID {.id = _type_id++ };
    types.insert(std::make_pair(id, type));
    type_names.insert(std::make_pair(type.name, id));
    return id;
  }

  def::ValueID construct(const def::TypeID type_id, const def::ArgumentList arguments = {}) {
    const auto type = types.at(type_id);
    if (type.constructor.is_abstract) { throw std::runtime_error("Tried to construct an abstract type"); }

    const def::ValueID id = {.id=_value_id++};
    values.insert(std::make_pair(id, std::make_shared<def::Value>(type.constructor.make(type_id))));
    return id;
  }

  def::ValueID construct(const def::TypeID type_id, const def::value_t value) {
    const auto type = types.at(type_id);
    if (type.constructor.is_abstract) { throw std::runtime_error("Tried to construct an abstract type"); }

    const def::ValueID id = {.id=_value_id++};
    values.insert(std::make_pair(id, std::make_shared<def::Value>(type.constructor.make_1(type_id, unpack_value_t(value)))));
    return id;
  }

  def::TypeID type_named(const std::string name) const {
    if (!type_names.contains(name)) {
      throw std::runtime_error("Unknown type");
    }
    return type_names.at(name);
  }

  private:
  size_t _type_id {0};
  size_t _value_id {0};
};



// Nodes that the interpreter reads and turns into calls to the VM
namespace ast {
  struct BindingID {
    const size_t id;
    bool operator==(const BindingID& other) const { 
      return id == other.id;
    }
  };
  struct FunctionID {
    const size_t id;
    bool operator==(const FunctionID& other) const { 
      return id == other.id;
    }
  };

  using ID = std::variant<def::ID, BindingID, FunctionID>;

  struct Symbol; struct RefBinding; struct RefType; struct MakeScalar;
  struct MakeBinding; struct FunctionDefinition; struct FunctionBody;

  struct Symbol : public std::string {};

  struct RefBinding { const Symbol symbol; };
  struct RefType { const Symbol symbol; };
  struct MakeScalar { const Symbol type_name; const def::value_t value; };

  struct Expression;
  struct Body : public std::vector<Expression> {};
  struct FunctionDefinition {
    const Body body;
    // TODO: Shouldn't get to be null!
    const RefType return_type_id;
  };

  using InputValue = std::variant<RefBinding, RefType, MakeScalar, FunctionDefinition>;
  using IntermediateValue = std::variant<RefBinding, RefType, def::ID>;

  struct MakeBinding {
    const Symbol symbol;
    const InputValue value;
  }; 

  using ExpressionTypes = std::variant<MakeBinding>;
  struct Expression : public ExpressionTypes {
    Expression(MakeBinding b) : ExpressionTypes(b) {}
  };

  struct hash_id {
    std::size_t operator() (const BindingID &type) const { return type.id; }
    std::size_t operator() (const FunctionID &type) const { return type.id; }
    std::size_t operator() (const Symbol &symbol) const { return std::hash<std::string>{}(symbol); }
  };
};


std::string id_to_named_id(def::ID id) {
  if (std::holds_alternative<def::TypeID>(id)) {
    const auto type_id = std::get<def::TypeID>(id);
    return fmt::format("TypeID:{}", type_id.id);
  } else if (std::holds_alternative<def::ValueID>(id)) {
    const auto type_id = std::get<def::ValueID>(id);
    return fmt::format("ValueID:{}", type_id.id);
  } else {
    throw std::runtime_error("Fill me in\n");
  }
}
std::string id_to_named_id(ast::ID id) {
  if (std::holds_alternative<def::ID>(id)) {
    return id_to_named_id(std::get<def::ID>(id));
  } else if (std::holds_alternative<ast::BindingID>(id)) {
    const auto binding_id = std::get<ast::BindingID>(id);
    return fmt::format("BindingID:{}", binding_id.id);
  } else if (std::holds_alternative<ast::FunctionID>(id)) {
    const auto function_id = std::get<ast::FunctionID>(id);
    return fmt::format("FunctionID:{}", function_id.id);
  } else {
    throw std::runtime_error("Fill me in\n");
  }
}

struct Debugger;
struct Interpreter {
  Interpreter(VM& vm) : vm(vm) {}
  Interpreter(VM& vm, const Interpreter* parent) : vm(vm), parent(parent) {}

  auto find_type_by_name(const ast::Symbol name) const {
    if (vm.type_names.contains(name)) {
      return vm.type_names.at(name);
    }

    if (parent) {
      fmt::print("Looking into parent for type {}\n", name);
      return parent->find_type_by_name(name);
    }

    throw std::runtime_error("Could not find named type");
  }

  ast::ID operator()(const ast::RefType type) {
    return vm.type_named(type.symbol);
  }

  ast::ID operator()(const ast::MakeBinding binding) {
    size_t id = std::hash<std::string>{}(binding.symbol);
    const auto binding_id = ast::BindingID {.id=id};

    if (std::holds_alternative<ast::RefBinding>(binding.value)) {
      const auto ref = std::get<ast::RefBinding>(binding.value);
      if (ref.symbol == binding.symbol) {
        throw std::runtime_error(fmt::format("Cannot bind {} as a reference to itself", ref.symbol));
      }
      binding_values.insert(std::make_pair(binding_id, ref));

    } else if (std::holds_alternative<ast::RefType>(binding.value)) {
      const auto ref = std::get<ast::RefType>(binding.value);
      binding_values.insert(std::make_pair(binding_id, ref));

    } else if (std::holds_alternative<ast::MakeScalar>(binding.value)) {
      const auto make = std::get<ast::MakeScalar>(binding.value);
      const auto type_id = find_type_by_name(make.type_name);
      // const def::ID value_id = vm.construct(type_id, make.value);
      // const ast::IntermediateValue value = {value_id};
      // binding_values.insert(std::make_pair(binding_id, value));

    } else if (std::holds_alternative<ast::FunctionDefinition>(binding.value)) {
      return (*this)(std::get<ast::FunctionDefinition>(binding.value));
    }

    bindings.insert(std::make_pair(binding.symbol, binding_id));
    return binding_id;
  }

  ast::ID operator()(const ast::FunctionDefinition function) {
    ast::FunctionID id { .id=_function_id++ };
    functions.insert(std::make_pair(id, function));
    return id;
  }

  // TODO: This should be something like ast::FunctionCall(FunctionID, Arguments)
  ast::ID operator()(const ast::FunctionID id) {
    fmt::print("interpret: FunctionID\n");
    const auto function = functions.at(id);
    if (function.body.empty()) {
      fmt::print("warn: Empty function body\n");
      return vm.construct(UnitID);
    };

    VM scope_vm;
    auto scope = Interpreter(scope_vm, this);

    const auto exec = [&function, &scope_vm, &scope]() {
      if (function.body.size() == 1) { return std::visit(scope, function.body.front()); }

      auto cur = function.body.cbegin();
      const auto end = std::prev(function.body.cend());

      while (cur != end) {
        std::visit(scope, *cur);
        ++cur;
      }

      const auto last_value = std::visit(scope, *end);
      fmt::print("Return here\n");
      return last_value;
    };

    const auto value = exec();
    // OK now we have to store this in this scope (i.e. in our current VM)
    fmt::format("Execution returned {}", id_to_named_id(value));

    return value;
  }

  private:

  std::unordered_map<ast::BindingID, ast::IntermediateValue, ast::hash_id>      binding_values {};
  std::unordered_map<ast::Symbol, ast::BindingID, ast::hash_id>                 bindings {};
  std::unordered_map<ast::FunctionID, ast::FunctionDefinition, ast::hash_id>    functions {};
  size_t _function_id {0};

  VM& vm;
  const Interpreter* parent;

  friend class Debugger;
};

struct Debugger {
  Debugger(const VM& vm, const Interpreter& interpret) : vm(vm), interpreter(interpret) {}

  void operator()(const def::TypeID type_id) {
    const auto type = vm.types.at(type_id);
    fmt::print("debug:type        ({}, {})\n", type.name, type.constructor.is_abstract ? "Abstract" : "Concrete");
  }

  void operator()(const def::ValueID value_id) {
    const auto value = vm.values.at(value_id);
    const auto type = vm.types.at(value->type_id);
    fmt::print("debug:val         ({}, {}, [use_count:{}])\n", type.name, value->v, value.use_count() - 1);
  }

  void operator()(const ast::BindingID binding_id) {
    std::optional<ast::IntermediateValue> value;
    if (interpreter.binding_values.contains(binding_id)) {
      value.emplace(interpreter.binding_values.at(binding_id));
    }

    if (!value)
      throw std::runtime_error("Invalid binding_id, interpret has no such binding.");

    (*this)(value.value());
  }

  void operator()(const ast::RefBinding binding) {
    fmt::print("ast:RefBinding '{}' -> ", binding.symbol);
    const auto id = interpreter.bindings.at(binding.symbol);
    const auto value = interpreter.binding_values.at(id);
    (*this)(value);
  }

  void operator()(const ast::RefType type_ref) {
    fmt::print("ast:RefType {}\n", type_ref.symbol);
    const auto type_id = vm.type_names.at(type_ref.symbol);
    (*this)(type_id);
  }

  void operator()(const ast::IntermediateValue value) {
    if (std::holds_alternative<def::ID>(value)) {
      const auto vm_id = std::get<def::ID>(value);
      fmt::print("ast:IntermediateValue -> vm:ID {}\n", id_to_named_id(vm_id));
      (*this)(vm_id);
      return;
    }
    fmt::print("ast:IntermediateValue -> ");
    std::visit(*this, value);
  }

  void operator()(const ast::FunctionID function_id) {
    const auto function = interpreter.functions.at(function_id);
    fmt::print("debug:function    ");
    std::string s = "(TODO: MAKE ME)";
    // std::string s = "(";
    // for (const auto type_id : function.parameters) {
    //   s.append(id_to_named_id(def::ID(type_id)));
    //   s.append(", ");
    // }
    // if (!function.parameters.empty()) s.erase(s.end() - 2, s.end());

    s.append(fmt::format(") -> {}", function.return_type_id.symbol));
    fmt::print("{}\n", s);
  }


  void operator()(const ast::ID id) {
    fmt::print("ast:ID -> ");
    if (std::holds_alternative<ast::FunctionID>(id)) {
      fmt::print("ast:FunctionID\n");
    }
    std::visit(*this, id);
  }

  void operator()(const def::ID id) {
    std::visit(*this, id);
  }

  private:
  const VM& vm;
  const Interpreter& interpreter;
};

struct Language {
  Language(Interpreter &i) : interpret(i) {}

  auto ref_type(std::string name) { return ast::RefType { .symbol = name }; }
  auto ref_binding(std::string name) { return ast::RefBinding {.symbol = name}; }

  auto make_binding(std::string name, ast::InputValue value) {
    return ast::MakeBinding {
      .symbol = name,
      .value = value
    };
  }

  auto make_function(ast::RefType return_type, ast::Body body) {
    return ast::FunctionDefinition {
      .body = body,
      .return_type_id = return_type
    };
  }

  template<typename T>
  ast::MakeScalar make_scalar(T v) {
    throw std::runtime_error("lang: Don't know hot to make scalar of this kind, add another case below");
  }

  template<>
  ast::MakeScalar make_scalar(int64_t i) {
    return ast::MakeScalar {
       .type_name = "Int",
       .value = i
    };
  }

  Interpreter &interpret;
};

int main() {
  VM root;
  root.register_type(Unit);
  root.register_type(Int);
  root.register_type(Char);
  root.register_type(Function);

  Interpreter interpret(root);
  Language lang(interpret);
  Debugger debug(root, interpret);

  // fmt::print("---- make_binding\n");
  // fmt::print("---- ---- x = Int\n");
  // const auto binding_id_x = interpret(lang.make_binding("x", lang.ref_type("Int")));
  // debug(binding_id_x);
  // fmt::print("---- ---- y = x\n");
  // const auto binding_id_y = interpret(lang.make_binding("y", lang.ref_binding("x")));
  // debug(binding_id_y);

  // fmt::print("---- ---- four = Int(4)\n");
  // const auto binding_id_four = interpret(lang.make_binding("four", lang.make_scalar<int64_t>(4)));
  // debug(binding_id_four);

  // fmt::print("---- ---- also_four = four\n");
  // const auto binding_id_also_four = interpret(lang.make_binding("also_four", lang.ref_binding("four")));
  // debug(binding_id_also_four);

  fmt::print("---- ast::FunctionDefinition\n");
  const auto function_1 = interpret(lang.make_function(
    lang.ref_type("Int"),
    {{
      lang.make_binding("four", lang.make_scalar<int64_t>(4)),
      // lang.make_binding("four_also", lang.ref_binding("four"))
      // lang.make_binding("four", lang.ref_type("Int"))
    }}
  ));
  debug(function_1);
  const auto return_value_1 = interpret(std::get<ast::FunctionID>(function_1));
  // debug(return_value_1);
  // const auto binding_function_1 = interpret(
  //   ast::MakeBinding {
  //     ast::Symbol{"foo"},
  //     ast::InputValue { ast::FunctionDefinition {
  //       // ast::MakeBinding { "x", MakeScalar { "Int", }}
  //     }}
  //   }
  // );

  // fmt::print("---- def::Type\n");
  // debug(UnitID);
  // debug(IntID);
  // debug(CharID);

  // fmt::print("\n\n");
  // fmt::print("---- def::Construct\n");
  // const auto int_value_id = vm.interpret(def::Construct {
  //   .arguments = {},
  //   .type_id = IntID
  // });
  // debug(int_value_id);

  // const auto char_value_id = vm.interpret(def::Construct {
  //   .arguments = {},
  //   .type_id = CharID
  // });
  // debug(char_value_id);

  // const auto int_value_four_id = vm.interpret(def::Manifest {
  //   .value = def::Value(IntID, int64_t{44447})
  // });
  // debug(int_value_four_id);

  // fmt::print("\n\n");
  // fmt::print("---- def::Binding\n");
  // fmt::print("---- ---- Default constructed\n");
  // const auto binding_x = vm.interpret(def::NewBinding {
  //   .name = "x",
  //   .to_id = int_value_four_id
  // });
  // debug(binding_x);
  // const auto binding_y = vm.interpret(def::NewBinding {
  //   .name = "y",
  //   .to_id = char_value_id
  // });
  // debug(binding_y);

  // fmt::print("\n---- ---- Initialised with value\n");
  // const auto value_id_1 = def::ID(vm.construct(IntID, int64_t{4}));
  // const auto binding_initialised_int = vm.interpret(def::NewBinding {
  //   .name = "initialised_int",
  //   .to_id = value_id_1
  // });
  // debug(binding_initialised_int);

  // const auto value_id_a = def::ID(vm.construct(CharID, char{'a'}));
  // const auto binding_initialised_char = vm.interpret(def::NewBinding {
  //   .name = "initialised_char",
  //   .to_id = value_id_a
  // });
  // debug(binding_initialised_char);


  // fmt::print("----\n");
  // fmt::print("---- Functions\n");
  // fmt::print("---- ---- With empty scope\n");
  // fmt::print("---- ---- ---- With arguments\n");
  // const auto function_id_1 = vm.interpret(def::Function {
  //   .parameters = {{IntID, IntID}},
  //   .return_type_id = IntID
  // });
  // debug(function_id_1);


  // const auto function_binding_id_1 = vm.interpret(def::NewBinding {
  //   .name = "function_binding_1",
  //   .to_id = function_id_1,
  // });
  // debug(function_binding_id_1);


  // fmt::print("\n");
  // fmt::print("---- ---- With non-empty scope\n");
  // fmt::print("---- ---- ---- def::Manifest IntID 1024\n");
  // const auto function_id_2 = vm.interpret(def::Function {
  //   .parameters = {},
  //   .return_type_id = IntID,
  //   .scope = def::Scope {{
  //     def::Manifest {
  //       .value = def::Value(IntID, int64_t{1024})
  //     }
  //   }}
  // });
  // debug(function_id_2);
  // const auto return_value_id_2 = vm.invoke(std::get<def::FunctionID>(function_id_2));
  // debug(return_value_id_2);
  // fmt::print("---- ---- ---- manifest into binding and evaluate binding\n");
  // const auto function_id_3 = vm.interpret(def::Function {
  //   .parameters = {},
  //   .return_type_id = IntID,
  //   .scope = def::Scope {{
  //     def::NewBinding {
  //       .name = "xyz",
  //       .to_id = IntID
  //     }
  //   }}
  // });
  // const auto return_value_id_3 = vm.invoke(std::get<def::FunctionID>(function_id_3));
  // debug(return_value_id_3);

  // // fmt::print("\n");
  // // fmt::print("---- ---- With parameters\n");
  // // const auto function_id_3 = vm.register_function(def::Function {
  // //   .parameters = {{IntID}},
  // //   .return_type_id = IntID,
  // //   .scope = def::Scope {{
  // //     def::Construct {
  // //       .type_id = IntID,
  // //       .arguments {{
  // //         def::Value(IntID, int64_t{1024})
  // //       }}          
  // //     }
  // //   }}
  // // });
  return 0;
}