#include<string>
#include<vector>
#include<cstddef>
#include<concepts>
#include<iostream>
#include<algorithm>
#include<optional>
#include<unordered_map>
#include<variant>
#include "result.h"

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


// All the glorious value types
namespace Val {
  using Int = int;
  using Str = std::string;

  using Any = std::variant<Int, Str>;
}

/**
Staircase<Scope<Type>> |__
                          |__
                             |__

Where every _ is a Type
And every | (move down) is a new scope
The horizontal movement indicates "as we move through the code"
Leaving a scope means we move vertically up, horizontally left

Each Type that derives of another Type has a pointer to its parent. Scalar types (those at the highest level)
point to nothing (beware!).
*/
template<typename T>
using Staircase = std::vector<T>;

template<typename T, typename U>
using Map = std::unordered_map<T, U>;

template<typename T>
using Scope = std::vector<T>;

template<typename T>
using Pointer = std::shared_ptr<const T>;

struct Type {
  const std::string name {""};
  const std::optional<Pointer<Type>> of;

  const std::optional<std::function<Val::Any(const std::string&)>> from_str;
};

struct Symbol {
  const std::string name;
  Pointer<Type> type;
};

namespace Expr {
  namespace Declare {
    struct Binding { std::string name; std::string type; std::string value_expr; }; 
    struct Type { std::string name; std::string of; };
  }

  namespace Eval {
    struct Binding { std::string name; };

    struct Apply {
      std::string func;
      std::vector<Binding> parameters;
    };
  }

  using Any = std::variant<Declare::Binding, Declare::Type, Eval::Binding>;
}

namespace Types {
  namespace Top {
    static const Pointer<Type> Int = std::make_shared<Type>(Type {
      .name = "Int", 
      .from_str = [](const auto& s) { return stoi(s); } 
    });
    static const Pointer<Type> Str = std::make_shared<Type>(Type{
      .name = "Str",
      .from_str = [](const auto& s) { return s; }
    });
  } 

  bool is_assignable(const std::string& val, const Type& t) {
    if (t.name == "Int")
      return std::all_of(val.begin(), val.end(), ::isdigit);

    if (t.name == "Str")
      return true;

    if (t.of) {
      return is_assignable(val, *(t.of.value()));
    }

    return false;
  }


  template<typename T>
  void assert_exists(const std::string& name, const std::optional<Pointer<T>>& t) {
    if (t) return;
    std::string message = "TypeAssert error: There is no type called ";
    message.append(name);
    throw std::runtime_error(message);
  }

  void assert_assignable(const std::string& val, const Type& t) {
    if (is_assignable(val, t)) return;
    std::string message = "TypeAssert error: ";
    message.append(val);
    message.append(" cannot be assigned to type ");
    message.append(t.name);
    throw std::runtime_error(message);
  }
}


static const Scope<Pointer<Type>> default_types  = Scope<Pointer<Type>>{
  Types::Top::Int,
  Types::Top::Str,
};

struct Graph {
  // These vectors must at all times have at least one element
  Staircase<Scope<Pointer<Symbol>>>     symbols { {} };
  Staircase<Scope<Pointer<Type>>>       types { {default_types}, {} };

  std::optional<Pointer<Type>> find_type(const std::string ofName) {
    for (const auto& scope : types) {
      for (const auto& type : scope) {
        if (type->name == ofName) return type;
      }
    }
    return {};
  }

  Pointer<Symbol> insert(Expr::Declare::Binding expr) {
    const auto t = find_type(expr.type);
    Types::assert_exists(expr.type, t);
    Types::assert_assignable(expr.value_expr, *(t.value()));

    auto& current_scope = symbols.back();
    current_scope.push_back( std::make_shared<Symbol>(Symbol {
      .name = expr.name,
      .type = *(t)
    }) );
    return current_scope.back();
  }

  Pointer<Type> insert(Expr::Declare::Type expr) {
    std::optional<Pointer<Type>> parent_pointer {};

    if (expr.of.length() > 0) {
      const auto t = find_type(expr.of);
      Types::assert_exists(expr.of, t);
      parent_pointer = t;
    }

    auto& current_scope = types.back();
    current_scope.push_back( std::make_shared<Type>(Type {
      .name = expr.name,
      .of = parent_pointer 
    }));
    return current_scope.back();
  }
};

std::ostream& operator<<(std::ostream& out, const Pointer<Symbol> symbol) {
  out << symbol->name;
  if (symbol->type) {
    out << "(" << symbol->type->name << ")";
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const Pointer<Type>& type) {
  out << type->name;
  return out;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const Staircase<Scope<T>>& stair_case) {
  std::string prefix = "";
  for (const Scope<T>& scope : stair_case) {
    out << prefix;
    for (const T& t : scope) {
      out << t << " ";
    }
    out << "\n";
  }
  return out;
}

struct VM {
  Staircase<Map<std::string, Pointer<Val::Any>>> symbol_values { {} };

  std::string debug_value(const Pointer<Symbol>& s) {
    // Look in every scope foo!
    const auto& current_scope = symbol_values.back();
    if (current_scope.count(s->name) == 0) {
      return std::string(s->name + " : unknown value");
    }
    // This throws
    const auto ptr = current_scope.at(s->name);

    return s->name + " : " + std::visit(overloaded {
      [](const Val::Str& str) { return str; },
      [](const Val::Int& v) { return std::to_string(v); },
    }, 
      *ptr
    );
  }
};

struct Evaluator {
  VM& vm;
  Graph& graph;

  Evaluator(VM& v, Graph& g) : vm(v) , graph(g) {};

  Pointer<Type> operator()(Expr::Declare::Type& type_decl) {
    return graph.insert(type_decl);
  }
  
  Pointer<Symbol> operator()(Expr::Declare::Binding& bind_decl) {
    auto b = graph.insert(bind_decl);
    auto t = b->type;

    if (!t->from_str) {
      auto candidate = t->of;
      while (candidate) {
        auto candidate_val = candidate.value();
        if (candidate_val->from_str) t = candidate_val;
        candidate = candidate_val->of;
      }
    }

    if (!t->from_str) throw std::runtime_error("Encountered type without from_str " + b->type->name);

    const auto v = t->from_str.value();
    auto& current_scope = vm.symbol_values.back();
    current_scope.insert(std::make_pair(b->name, std::make_shared<Val::Any>(v(bind_decl.value_expr))));

    return b;
  }

  void operator()(Expr::Eval::Apply& apply_decl) {

  }
};

int main() {
  VM vm;
  Graph g;
  Evaluator eval(vm, g);

  try {
    auto type_expr = Expr::Declare::Type {.name = "ID", .of = "Int"};
    auto t = eval(type_expr);

    auto bind_expr = Expr::Declare::Binding {.type = "ID", .name = "x", .value_expr = "42"};
    auto b = eval(bind_expr);
    std::cout << vm.debug_value(b) << '\n';
    
    auto b2_expr = Expr::Declare::Binding {.type = "Str", .name = "foo", .value_expr = "fighters are OK"};
    auto b2 = eval(b2_expr);
    std::cout << vm.debug_value(b2) << '\n';

    auto app_1 = Expr::Eval::Apply {.func = "+", .parameters = { Expr::Eval::Binding {"x"}, Expr::Eval::Binding {"x"} }};

  } catch (const std::runtime_error& err) {
    std::cout << "Error during compilation: \n\n" << err.what() << "\n";
    return 0;
  }

  // std::cout << "types:\n" << g.types << "\n";
  // std::cout << "symbs:\n" << g.symbols << "\n";

  return 0;
}