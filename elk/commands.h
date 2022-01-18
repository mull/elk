#pragma once

#include "gc.h"
#include "scope.h"

std::size_t construct(GC&, std::size_t TypeID, std::size_t TypeConstructorID);
std::size_t bind(Scope&, std::size_t, Binding);


struct NewType {
  std::optional<std::size_t> base { std::nullopt };
};

void assert_type_exists(const GC& gc, std::size_t id) {
  
}

TypePtr create_type(GC& gc, NewType definition) {
  if (definition.base) {
    assert_type_exists(gc, definition.base.value());
  }
  return {.id = 1};
}