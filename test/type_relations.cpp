#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "../elk/compiler/hoot.hpp"
#include "../elk/compiler/Graph.hpp"
#include "../elk/compiler/defs.hpp"

using namespace elk::compiler;
using namespace elk::compiler::hoot;

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

TEST_CASE ("type conversions" ) {
  SECTION ( "id: Int <-> Int") {
    REQUIRE ( conversion_paths(graph, Int, Int) == true );
  };

  SECTION ( "subtype: ID . Int " ) {
    SECTION ( "id: ID -> ID" ) {
      REQUIRE ( conversion_paths(graph, ID, ID) == true );
    }

    SECTION ( "relation:parent" ) {
      REQUIRE ( conversion_paths(graph, Int, ID) == true );
      REQUIRE ( conversion_paths(graph, ID, Int) == true );
    }

    SECTION ( "relation:parent" ) {
      auto UserID = graph.insert_type({.name = "UserID", .of = {ID}});
      auto descendants = graph.descendants_of(ID);
      REQUIRE (descendants.size() == 1);
      REQUIRE (descendants[0] == UserID);
    }

    SECTION ( "relation:siblings" ) {
      auto PK = graph.insert_type({ .name = "PK", .of = { Int }});

      /**
         Int
        /   \
      ID    PK 

      if Id -> Int is possible
      and Int -> PK is possible
      then ID -> PK is possible through composition (path extension)
      */

      // ID -> Int -> PK
      REQUIRE ( conversion_paths(graph, ID, PK) == true );
      // PK -> Int -> ID
      REQUIRE ( conversion_paths(graph, PK, Int) == true );
    }
  };
}