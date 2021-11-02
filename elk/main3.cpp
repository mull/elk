#include <any>
#include <cstdint>
#include <list>
#include <unordered_map>
#include <variant>
#include <tuple>
#include <string>
#include <optional>
#include "fmt/core.h"

namespace Memory { 
  using ID = std::size_t; 
};

struct Type {
  std::string name;
};

using IntLiteral = int64_t;
using CharLiteral = char;

using Literal = std::variant<std::nullopt_t, IntLiteral, CharLiteral>;

struct Parameter {
  const std::string name {""};
  const Memory::ID kind {};
};
struct Argument {
  const std::string name {""};
  
};

struct ParameterList : public std::list<Parameter> {}; 
struct ArgumentList : public std::list<Argument> {};

struct TypeRef {
  TypeRef(std::string&& name) : name(name) {}
  std::string name;
};


struct CallNativeConstructor  {
  const TypeRef of;
  const Literal with { std::nullopt };

  CallNativeConstructor(TypeRef r) : of(r) {};
  CallNativeConstructor(TypeRef r, Literal w) : of(r), with(w) {};
};

struct MakeBinding {
  const std::string name;
};

struct Value : public std::variant<int64_t, char> {};
using NativeConstructor = std::function<Value(const ArgumentList)>;

struct Constructor {
  const NativeConstructor callable;
  const ParameterList parameters {};
};

struct GC {
  std::unordered_map<Memory::ID, std::shared_ptr<Value>> refs {};

  Memory::ID construct(NativeConstructor c, ArgumentList) {
    fmt::print("scope.construct.(NativeConstructor)\n");
    const auto id = Memory::ID(++_value_id);
    refs.insert(std::make_pair(id, std::make_shared<Value>(c(ArgumentList{}))));
    return id;
  }

  private:
  size_t _value_id {0};
};

struct Scope {
  std::unordered_map<std::string, Memory::ID> type_names {};
  std::unordered_map<Memory::ID, Type> types {};
  std::unordered_map<Memory::ID, std::list<Constructor>> type_constructors {};

  std::list<Memory::ID> references {};

  ~Scope() {
    if (!references.empty()) {
      fmt::print("warn: Scope still has {} references upon destruction\n", references.size());
    }
  }
};

template<typename T>
struct Result : public std::variant<std::string, T> {
  operator bool() const {
    return std::holds_alternative<T>(*this);
  }

  T value() const {
    if (!(*this)) throw std::runtime_error("Result holds Error, not Value");
    return std::get<T>(*this);
  }

  std::string error() const {
    if ((*this)) throw std::runtime_error("Result holds Value, not Error");
    return std::get<std::string>(*this);
  }

  template<typename TNext>
  Result<TNext> map(std::function<Result<TNext>(const T in)> fn) const {
    if (*this) {
      return fn(this->value());
    }
    return {std::get<std::string>(*this)};
  }
};

struct Query {
  const Scope& scope;

  Result<Memory::ID> find_type_id(const TypeRef ref) const { 
    const auto id = scope.type_names.find(ref.name);
    if (id == scope.type_names.cend()) {
      return { fmt::format("No type with name {} found", ref.name) };
    };
    return { id->second };
  }

  Result<Constructor> find_matching_constructor(const Memory::ID type_id, ArgumentList args) const {
    const auto it = scope.type_constructors.find(type_id);
    if (it == scope.type_constructors.end()) {
      return { fmt::format("There are no constructors for type_id:{}", type_id) };
    }

    const auto& constructors = it->second;
    fmt::print("{} constructors for that type\n", constructors.size());
    for (const auto c : constructors) {
      if (c.parameters.size() == args.size()) {
        return {c};
      }
    }

    return { fmt::format("No constructor matching those arguments") };
  }
};


using Effect = std::function<Memory::ID(Scope&, GC&)>;
static const Effect noop = [](auto&, auto&) { return Memory::ID(0); };

struct Traverse {
  GC& gc;
  Scope& scope;
  const Query q = Query{scope};

  void operator()(const CallNativeConstructor construct) {
    const auto type_id = q.find_type_id(construct.of);
    if (!type_id) {
      fmt::print("Traverse.type_id_lookup: {}\n", type_id.error());
      return; 
    }

    const auto constructor = q.find_matching_constructor(type_id.value(), ArgumentList {});
    if (!constructor) {
      fmt::print("Traverse.find_matching_constructor: {}\n", constructor.error());
      return;
    }

    const auto native = constructor.value().callable;
    const auto id = gc.construct(native, ArgumentList {});
    scope.references.push_back(id);
  }

  void operator()(const MakeBinding make_binding) {

  }
};

void print_scope(Scope&);

int main() {
  GC gc;
  Scope root;

  root.types.insert(std::make_pair(0, Type{"Void"}));
  root.type_names.insert(std::make_pair("Void", 0));
  root.types.insert(std::make_pair(1, Type{"Int"}));
  root.type_names.insert(std::make_pair("Int", 1));
  root.types.insert(std::make_pair(2, Type{"Char"}));
  root.type_names.insert(std::make_pair("Char", 2));

  {
    auto& constructors = root.type_constructors[1]; // Int
    // constructors.push_back({[](auto) -> Value { return {int64_t{0}}; }});
    auto f = Constructor {
      .callable = [](ArgumentList args) -> Value {
        return { int64_t{1} };
      },
      .parameters = {{
        {"x", 1}
      }}
    };
    constructors.push_back(f);
  }

  {
    auto& constructors = root.type_constructors[2]; // Char
    NativeConstructor constructor = [](auto) -> Value { return {'0'}; };
    constructors.push_back({[](auto) -> Value { return {'0'}; }});
  }

  Traverse traverse{gc, root};
  traverse(CallNativeConstructor(TypeRef("Int")));
  traverse(CallNativeConstructor(TypeRef("Int"), {int64_t{1337}}));

  // trav(
  //   MakeBinding{
  //     .name = "x",
  //     .to = {TypeRef("Int")}
  //   }
  // );

  print_scope(root);
}


void print_scope(Scope& scope) {
  fmt::print("------ scope\n");
  fmt::print("{} references\n", scope.references.size());
  fmt::print("{} types and {} constructors\n", scope.types.size(), scope.type_constructors.size());
  fmt::print("------ //scope\n");
}