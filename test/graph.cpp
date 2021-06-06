#include "catch2/catch.hpp"

#include "../elk/compiler/Graph.hpp"
#include "../elk/compiler/defs.hpp"

using namespace elk::compiler;

namespace {
  Graph graph;
  auto Int = graph.insert_type({
    .name = "Int"
  });
  auto ID = graph.insert_type({
    .name = "ID",
    .of = { Int }
  });
}

TEST_CASE ("graph" ) {
  SECTION ( "siblings_of(Child)" ) {
    auto OtherID = graph.insert_type({ .name = "OtherID", .of = { Int }});
    auto siblings = graph.siblings_of(ID);
    REQUIRE (siblings.size() == 1);
    REQUIRE ( siblings[0] == OtherID );
  }

  SECTION ( "descendants_of(Parent)" ) {
    auto UserID = graph.insert_type({.name = "UserID", .of = {ID}});
    auto descendants = graph.descendants_of(ID);
    REQUIRE (descendants.size() == 1);
    REQUIRE (descendants[0] == UserID);
  }
}