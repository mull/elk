#pragma once

#include "gc.h"

#include <unordered_map>
#include <memory>
#include <variant>

struct Binding : public std::variant<ValuePtr, FunctionPtr, TypeConstructorPtr, TypePtr> {
  bool is_value() const noexcept {
    return std::holds_alternative<ValuePtr>(*this);
  }

  bool is_function() const noexcept {
    return std::holds_alternative<FunctionPtr>(*this);
  }

  bool is_constructor() const noexcept {
    return std::holds_alternative<TypeConstructorPtr>(*this);
  }

  bool is_type() const noexcept {
    return std::holds_alternative<TypePtr>(*this);
  }
};

// Commodity
class Scope {
  std::unordered_map<std::size_t, Binding> bindings;
};