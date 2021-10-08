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
  std::unordered_map<def::ValueID, VMValue, def::hash_id>   values {};
  std::optional<const VM*>                                  parent;

  VM() = default;

  def::TypeID register_type(const def::Type type) {
    const auto id = def::TypeID {.id = _type_id++ };
    types.insert(std::make_pair(id, type));
    return id;
  }

  def::ValueID construct(const def::TypeID type_id) {
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

  template<typename ItemType>
  struct TupleItem {
    const Symbol        symbol;
    const ItemType      item;
  };

  template<typename ItemType>
  struct Tuple : public std::vector<TupleItem<ItemType>> {};

  using FunctionParams = Tuple<def::TypeID>;
  using FunctionArguments = Tuple<ast::ID>;

  struct FunctionDefinition {
    const Body body;
    const FunctionParams parameters;
    // TODO: Shouldn't get to be null!
    const RefType return_type_id;
  };

  using InputValue = std::variant<RefBinding, RefType, MakeScalar, FunctionDefinition, def::ValueID>;

  struct MakeBinding {
    const Symbol symbol;
    const InputValue value;
  }; 

  using ExpressionTypes = std::variant<MakeBinding, RefBinding, RefType>;
  struct Expression : public ExpressionTypes {
    Expression(MakeBinding b) : ExpressionTypes(b) {}
    Expression(RefType b) : ExpressionTypes(b) {}
    Expression(RefBinding b) : ExpressionTypes(b) {}
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

struct Scope {
  std::unordered_map<ast::BindingID, ast::ID, ast::hash_id>                     binding_values {};
  std::unordered_map<ast::Symbol, ast::BindingID, ast::hash_id>                 binding_names {};
  std::unordered_map<std::string, def::TypeID>                                  type_names {};
  std::unordered_map<ast::FunctionID, ast::FunctionDefinition, ast::hash_id>    functions {};

  std::optional<std::shared_ptr<Scope>> parent {};
  Scope() = default;
  Scope(std::shared_ptr<Scope> parent) : parent(parent) {}

  void dump() const {
    fmt::print("----------------------------------------------------------------\n");
    fmt::print("Scope dump:\n");
    fmt::print("bindings\n");
    for (const auto b : binding_names) { 
      const auto value_id = binding_values.at(std::get<1>(b));
      fmt::print("\t{}: {} = {}\n", std::get<0>(b), id_to_named_id(std::get<1>(b)), id_to_named_id(value_id)); 
    }

    fmt::print("TODO: dump ({}) functions\n", functions.size());
    fmt::print("TODO: dump ({}) types\n", type_names.size());
    fmt::print("----------------------------------------------------------------\n");
  }
};

struct Interpreter {
  size_t _function_id {0};
  VM& vm;
  std::shared_ptr<Scope> scope { std::make_shared<Scope>() };

  Interpreter(VM& vm) : vm(vm) {}

  def::TypeID find_type_by_name(ast::Symbol name) const {
    const Scope* cur = scope.get();

    while (cur != NULL) {
      if (cur->type_names.contains(name)) {
        return cur->type_names.at(name);
      }

      cur = cur->parent->get();
    }

    throw std::runtime_error("Could not find named type");
  }

  ast::BindingID find_binding_by_name(ast::Symbol name) {
    const Scope* cur = scope.get();

    while (cur != NULL) {
      if (cur->binding_names.contains(name)) {
        return cur->binding_names.at(name);
      }

      cur = cur->parent->get();
    }

    throw std::runtime_error("Could not find named binding");
  }

  ast::ID resolve_binding(ast::BindingID id) {
    const Scope* cur = scope.get();
    while (cur != NULL) {
      if (cur->binding_values.contains(id)) {
        const auto val = cur->binding_values.at(id);
        if (std::holds_alternative<ast::BindingID>(val)) return resolve_binding(std::get<ast::BindingID>(val));
        return val;
      }
      cur = cur->parent->get();
    }
    throw std::runtime_error("Could not resolve binding");
  }

  ast::ID register_type(def::Type type) {
    const auto type_id = vm.register_type(type);
    scope->type_names.insert(std::make_pair(type.name, type_id));
    return type_id;
  }

  ast::ID operator()(ast::RefType type) {
    return find_type_by_name(type.symbol);
  }

  ast::ID operator()(ast::RefBinding binding) {
    return find_binding_by_name(binding.symbol);
  }

  ast::ID operator()(ast::MakeScalar scalar) {
      const auto type_id = find_type_by_name(scalar.type_name);
      return vm.construct(type_id, scalar.value);
  }

  ast::ID operator()(ast::MakeBinding binding) {
    // TODO: I think scope.id ought to be exist here or sth
    const size_t id = std::hash<std::string>{}(binding.symbol);
    const auto binding_id = ast::BindingID {.id=id};

    auto cur = scope->binding_names.find(binding.symbol);
    if (cur != scope->binding_names.end()) {
      scope->binding_names.erase(cur);
      // If that exists then there's also a binding value
      scope->binding_values.erase(scope->binding_values.find((cur->second)));
    }

    const ast::ID value_id = std::visit(overloaded {
      [this](const ast::FunctionDefinition def) -> ast::ID {
        const auto function_id = (*this)(def);
        return function_id;
      },
      [this, &binding](const ast::RefBinding ref) -> ast::ID {
        if (ref.symbol == binding.symbol) {
          throw std::runtime_error(fmt::format("Cannot bind {} as a reference to itself", ref.symbol));
        }
        const auto ref_id = find_binding_by_name(ref.symbol);
        return ref_id;
      },
      [this](const ast::RefType ref) -> ast::ID {
        const auto ref_id = find_type_by_name(ref.symbol);
        return ref_id;
      },
      [this](const ast::MakeScalar make) -> ast::ID {
        return (*this)(make);
      },
      [this](const def::ValueID id) -> ast::ID { return id; }
    }, binding.value);


    scope->binding_values.insert(std::make_pair(binding_id, value_id));
    scope->binding_names.insert(std::make_pair(binding.symbol, binding_id));
    return binding_id;
  }

  ast::ID operator()(ast::FunctionDefinition function) {
    if (function.body.empty()) {
      fmt::print("warn: Empty function body\n");
      return vm.construct(UnitID);
    };
    ast::FunctionID id { .id=_function_id++ };
    scope->functions.insert(std::make_pair(id, function));
    return id;
  }

  def::ID resolve_to_def_id(ast::ID ast_id) {
    if (std::holds_alternative<def::ID>(ast_id)) return std::get<def::ID>(ast_id);
    throw std::runtime_error("resolve: implement the rest of me");
  }

  def::ValueID resolve_to_value_id(def::ID def_id) {
    if (std::holds_alternative<def::ValueID>(def_id)) return std::get<def::ValueID>(def_id);
    return vm.construct(std::get<def::TypeID>(def_id));
  }

  def::TypeID resolve_type(ast::ID id) {
    if (std::holds_alternative<def::ID>(id)) { 
      const auto def_id = std::get<def::ID>(id);
      if (std::holds_alternative<def::TypeID>(def_id)) return std::get<def::TypeID>(def_id);

      if (std::holds_alternative<def::ValueID>(def_id)) {
        const auto val_id = std::get<def::ValueID>(def_id);
        const auto val = vm.values.at(val_id);
        return val->type_id;
      }
    }
    throw std::runtime_error("resolve_type: I know almost nothing");
  }

  // TODO: This should be something like ast::FunctionCall(FunctionID, Arguments)
  ast::ID operator()(ast::FunctionID id, ast::FunctionArguments arguments = {}) {
    const auto function = scope->functions.at(id);

    if (function.parameters.size() != arguments.size()) {
      throw std::runtime_error("Not the right amount of arguments to function");
    }

    std::optional<ast::ID> ret = UnitID;
    scope = std::make_shared<Scope>(scope);
    auto param = function.parameters.cbegin();
    auto arg = arguments.cbegin();

    while (param != function.parameters.cend()) {
      const auto param_type = param->item;
      const auto arg_actual = resolve_to_def_id(arg->item);
      const auto arg_type_id = resolve_type(arg_actual);

      if (arg_type_id != param_type) {
        throw std::runtime_error(
          fmt::format("Mismatch between types: arg_type.id:{} <> param_type.id:{}", arg_type_id.id, param_type.id)
        );
      }

      const auto arg_value = resolve_to_value_id(arg_actual);
      const auto v = (*this)(ast::MakeBinding {
        .symbol = param->symbol,
        .value = { arg_value }
      });

      param++; arg++;

      if (param == function.parameters.cend()) {
        ret.emplace(v);
      }
    }

    const auto value = manifest(ret.value());
    scope = scope->parent.value();
    return value;
  }

  // Ensure that the values of BindingID and FunctionID 
  // are manifested in the VM, and change the ast::ID to
  // point to the value (say the value of the binding) instead
  ast::ID manifest(ast::ID id) {
    return std::visit(overloaded {
      [id, this](ast::BindingID binding_id) -> ast::ID {
        const auto value = scope->binding_values.at(binding_id); 
        return manifest(value);
      },
      [this](ast::FunctionID function_id) -> ast::ID {
        throw std::runtime_error("Oh nein!\n");
      },
      [](def::ID id) -> ast::ID { 
        return id; 
      }
    }, id);
  }

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
    std::optional<ast::ID> value;
    if (interpreter.scope->binding_values.contains(binding_id)) {
      value.emplace(interpreter.scope->binding_values.at(binding_id));
    }

    if (!value)
      throw std::runtime_error("Invalid binding_id, interpret has no such binding.");

    (*this)(value.value());
  }

  void operator()(const ast::RefBinding binding) {
    fmt::print("ast:RefBinding '{}' -> ", binding.symbol);
    const auto id = interpreter.scope->binding_names.at(binding.symbol);
    const auto value = interpreter.scope->binding_values.at(id);
    (*this)(value);
  }

  void operator()(const ast::RefType type_ref) {
    fmt::print("ast:RefType {}\n", type_ref.symbol);
    const auto type_id = interpreter.find_type_by_name(type_ref.symbol);
    (*this)(type_id);
  }

  void operator()(const ast::FunctionID function_id) {
    const auto function = interpreter.scope->functions.at(function_id);
    fmt::print("debug:function    ");
    std::string s = "(";
    for (const ast::TupleItem param : function.parameters) {
      if (param.symbol.length() > 0) {
        s.append(param.symbol);
        s.append(": ");
      }
      const auto type = vm.types.at(param.item);
      s.append(type.name);

      s.append(", ");
    }
    if (!function.parameters.empty()) s.erase(s.end() - 2, s.end());

    s.append(fmt::format(") -> {}", function.return_type_id.symbol));
    fmt::print("{}\n", s);
  }


  void operator()(const ast::ID id) {
    std::visit(overloaded {
      [](ast::FunctionID) { fmt::print("ast:FunctionID\n"); },
      [](ast::BindingID) { fmt::print("ast:BindingID -> "); },
      [](auto) {}
    }, id);
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

  auto make_function(ast::RefType return_type, ast::Tuple<def::TypeID> params, ast::Body body) {
    return ast::FunctionDefinition {
      .body = body,
      .return_type_id = return_type,
      .parameters = params
    };
  }

  ast::ID resolve_to_value(ast::ID id) {
    if (std::holds_alternative<ast::BindingID>(id)) {
      return resolve_to_value(interpret.scope->binding_values.at(std::get<ast::BindingID>(id)));
    }
    return id;
  }

  auto make_function_call(std::string bound_name) {
    const auto binding_id = interpret.find_binding_by_name(ast::Symbol { bound_name });
    const auto binding_value = resolve_to_value(binding_id);

    if (std::holds_alternative<ast::FunctionID>(binding_value) == false) {
      throw std::runtime_error("lang: Binding does not point to a function");
    }
    return std::get<ast::FunctionID>(binding_value);
  }

  auto type(std::string name) {
    return interpret.find_type_by_name(ast::Symbol { name });
  }

  template<typename T>
  ast::MakeScalar make_scalar(T v) {
    throw std::runtime_error("lang: Don't know to to make scalar of this kind, add another case below");
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
  VM vm;
  Interpreter interpret(vm);

  interpret.register_type(Unit);
  interpret.register_type(Int);
  interpret.register_type(Char);

  Language lang(interpret);
  Debugger debug(vm, interpret);


  fmt::print("---- let echo(in: Int) -> in\n");
  fmt::print("---- echo(42)\n");
  const auto echo_fn_id = interpret(lang.make_function(
    lang.ref_type("Int"),
    ast::FunctionParams {{ 
      { .symbol = "in", .item = lang.type("Int") }
     }},
    ast::Body {{ 
      lang.ref_binding("in")
    }}
  ));
  debug(echo_fn_id);

  const auto four = interpret(lang.make_scalar<int64_t>(42));
  const auto return_value = interpret(
    std::get<ast::FunctionID>(echo_fn_id),
    ast::FunctionArguments {{ 
      { .symbol = "in", .item = four } 
    }}
  );
  debug(return_value);

  /*
  fmt::print("---- debug(T)\n");
  debug(UnitID);
  debug(IntID);
  debug(CharID);

  fmt::print("\n");
  fmt::print("---- make_binding\n");
  fmt::print("---- ---- x = Int\n");
  const auto binding_id_x = interpret(lang.make_binding("x", lang.ref_type("Int")));
  debug(binding_id_x);
  fmt::print("\n---- ---- y = x\n");
  const auto binding_id_y = interpret(lang.make_binding("y", lang.ref_binding("x")));
  debug(binding_id_y);

  fmt::print("\n---- ---- four = Int(4)\n");
  const auto binding_id_four = interpret(lang.make_binding("four", lang.make_scalar<int64_t>(4)));
  debug(binding_id_four);

  fmt::print("\n---- ---- also_four = four\n");
  const auto binding_id_also_four = interpret(lang.make_binding("also_four", lang.ref_binding("four")));
  debug(binding_id_also_four);

  fmt::print("\n---- make_function\n");
  const auto function_foo = interpret(lang.make_binding(
    "foo",
    lang.make_function(
      lang.ref_type("Int"),
      {{
        lang.make_binding("four", lang.make_scalar<int64_t>(4)),
        lang.make_binding("four_also", lang.ref_binding("four")),
        lang.make_binding("four_again", lang.ref_binding("four_also")),
        // lang.make_binding("four", lang.ref_type("Int"))
      }}
    )
  ));
  debug(function_foo);

  const auto return_value_foo = interpret(lang.make_function_call("foo"));
  debug(return_value_foo);

  
  fmt::print("\n---- bind to a function and call it\n");
  const auto bind_to_foo = interpret(lang.make_binding("foobar", lang.ref_binding("foo")));
  debug(bind_to_foo);
  debug(interpret(lang.make_function_call("foobar")));
  ;*/

  return 0;
}