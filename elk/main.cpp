#include "scope.h"
#include "gc.h"
#include "commands.h"

#include <iostream>

static const std::size_t IntID = 1;

int main() {
  std::cout << "elk init\n";
  GC gc;
  Scope scope;

  // construct(gc, IntID, 1);

  return 0;
}