#include "gc.h"

#include <iostream>

GC::GC() {
  std::cout << "GC init\n";
}

GC::~GC() {
  std::cout << "GC destruct: \n";
  std::cout << "\ttypes: " << types.size() << "\n";
}


std::optional<Type> GC::type(std::size_t id) const noexcept {
  if (!types.contains(id)) return {};
  return types.at(id);
}