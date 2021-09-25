#include <iostream>
#include <string>
#include <unordered_map>
#include <any>
#include <variant>
#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

namespace def {
  using value_t = std::variant<std::nullopt_t, int64_t, char>;
  struct Value; struct Type;

  struct Value {
    explicit Value(size_t type_id, int64_t i) : type_id(type_id), v(i) {};
    explicit Value(size_t type_id, char c) : type_id(type_id), v(c) {};


    const size_t type_id;
    const value_t v;
  };

  struct Constructor {
    const bool is_abstract { false };
    // TODO: This ought to fail somehow without exceptions
    const std::function<Value(size_t)>           make         = [](size_t type_id) -> Value { throw std::runtime_error("Can't construct an abstract type"); };
    const std::function<Value(size_t, std::any)> make_1       = [](size_t type_id, auto) -> Value { throw std::runtime_error("Can't construct an abstract type"); };
  };

  static Constructor make_int64_constructor() { 
    return Constructor {
      .is_abstract    = false,
      .make           = [](size_t type_id) { return Value(type_id, int64_t(0)); },
      .make_1         = [](size_t type_id, std::any) { return Value(type_id, int64_t(1)); }
    };
  }

  static Constructor make_char_constructor() { 
    return {
      .is_abstract  = false,
      .make         = [](size_t type_id) { return Value(type_id, char{'C'}); },
      .make_1       = [](size_t type_id, std::any) { return Value(type_id, char{'A'}); }
    };
  }

  struct Type {
    // TODO: id should never be a default value
    const size_t id;
    const std::string name;
    const def::Constructor constructor { .is_abstract = true };
  };

  struct Binding {
    const std::string name;
    const size_t type_id;
    const size_t value_id;
  };
};

template <> struct fmt::formatter<def::value_t>: formatter<std::string> {
  // parse is inherited from formatter<string>
  template <typename FormatContext>
  auto format(def::value_t value, FormatContext& ctx) {
    std::string name = std::visit(overloaded {
      [](int64_t i) { return fmt::to_string(i); },
      [](char c) { return fmt::to_string(c); },
      [](auto) { return fmt::to_string("???"); }
    }, value);

    return formatter<std::string>::format(name, ctx);
  }
};


static const auto Unit = def::Type { .id=0, .name = "Unit" };
static const auto Int = def::Type { .id=1, .name="Int", .constructor=def::make_int64_constructor() };
static const auto Char = def::Type { .id=2, .name="Char", .constructor=def::make_char_constructor() };

namespace op {
  def::Value plus(const def::Value& a, const def::Value& b) {
    return def::Value(a);
  }
};

struct VM {
  std::unordered_map<std::string, size_t> type_ids {
    std::make_pair("Unit", 0),
    std::make_pair("Int", 1),
    std::make_pair("Char", 2),
  };

  std::unordered_map<size_t, const def::Type> types {
    std::make_pair(0, Unit),
    std::make_pair(1, Int),
    std::make_pair(2, Char)
  };

  std::unordered_map<size_t, const def::Binding> bindings {};
  std::unordered_map<size_t, const def::Value> values {};


  void register_type(const def::Type type) {
    type_ids.insert(std::make_pair(type.name, type.id));
    types.insert(std::make_pair(type.id, type));
  }

  size_t register_binding(const def::Binding binding) {
    size_t id = std::hash<std::string>{}(binding.name);
    bindings.insert(std::make_pair(id, binding));
    return id;
  }

  size_t construct(const def::Type type) {
    if (type.constructor.is_abstract) { throw std::runtime_error("Tried to construct an abstract type"); }

    const auto id = _value_id++;
    values.insert(std::make_pair(id, type.constructor.make(type.id)));
    return id;
  }

  size_t construct(const def::Type type, def::value_t scalar_value) {
    if (type.constructor.is_abstract) { throw std::runtime_error("Tried to construct an abstract type"); }

    const auto id = _value_id++;
    values.insert(std::make_pair(id, type.constructor.make_1(type.id, scalar_value)));
    return id; 
  }

  const def::Binding& binding(size_t id) {
    return bindings.at(id);
  }

  const def::Value& value(size_t id) {
    return values.at(id);
  }

  void debug(const def::Type& type) {
    fmt::print("debug:type        ({}, {})\n", type.name, type.constructor.is_abstract ? "Abstract)" : "Concrete");
  }

  void debug(const def::Value& value) {
    const auto type = types.at(value.type_id);
    fmt::print("debug:val         ({}, {})\n", type.name, value.v);
  }

  void debug(const def::value_t& value) {
    fmt::print("{}", value);
  }

  void debug(const def::Binding& binding) {
    fmt::print("debug:binding     [name:{} type_id:{} value_id:{}]\n", binding.name, binding.type_id, binding.value_id);
    fmt::print("                  ");
    const auto type = types.at(binding.type_id);
    debug(type);
    const auto val = value(binding.value_id);
    fmt::print("                  ");
    debug(val);
  }


  private:
  size_t _value_id {0};
};



int main() {
  VM vm;
  vm.debug(Unit);
  // vm.construct(Unit);

  vm.debug(Int);
  const auto int_value_id = vm.construct(Int);
  vm.debug(vm.value(int_value_id));

  vm.debug(Char);
  const auto char_value_id = vm.construct(Char);
  vm.debug(vm.value(char_value_id));

  const auto binding_x = vm.register_binding(def::Binding {
    .name = "x",
    .type_id = Int.id,
    .value_id = int_value_id
  });
  vm.debug(vm.binding(binding_x));

  const auto binding_y = vm.register_binding(def::Binding {
    .name = "y",
    .type_id = Int.id,
    .value_id = int_value_id
  });
  vm.debug(vm.binding(binding_y));

  const auto value_id_1 = vm.construct(Int, int64_t{1});
  const auto binding_initialised = vm.register_binding(def::Binding {
    .name = "initialised",
    .type_id = Int.id,
    .value_id = value_id_1 
  });
  vm.debug(vm.binding(binding_initialised));

  return 0;
}