#pragma once

#include <unordered_map>

struct Value {};
struct Function {

};

struct TypeConstructor {
  std::size_t id;

  TypeConstructor() = delete;
};

struct Type {};

struct ValuePtr             { std::size_t id; };
struct FunctionPtr          { std::size_t id; };
struct TypeConstructorPtr   { std::size_t id; };
struct TypePtr              { std::size_t id; };

// Singleton
class GC {
public:
  GC();
  ~GC();

  std::optional<Type> type(std::size_t id) const noexcept;


private:
  std::unordered_map<size_t, Value>             values {};
  std::unordered_map<size_t, Function>          functions {};
  std::unordered_map<size_t, TypeConstructor>   type_constructors {};
  std::unordered_map<size_t, Type>              types {};
};
