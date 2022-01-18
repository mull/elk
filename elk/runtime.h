#pragma once

#include "gc.h"
#include <stdexcept>


struct NewType {
};

class Interpreter {
  const GC& gc;

  Interpreter(const GC& gc) : gc(gc) {}

  void assert_type_exists(std::size_t id) {
    const auto t = gc.type(id);
    if (!t) {
      throw std::runtime_error("FACK");
    }
  }


  TypePtr create_type(NewType definition) {
    return {.id = 1};
  }
};