lib/
  std/
    types/
      Int.cpp
      Str.cpp
      Rec.cpp
    interfaces/
      Reflexive.cpp
        An Object is reflexive if it exposes its internals
      Shapeable.cpp
        An object is shapeable if its Reflexive and some children are Shapeable | Convertible<Scalar>
    monads/
      Future.cpp
    io/
      Logger.cpp
        <<(Stringable<T>)
      Console.cpp
        <<(T)
        >>(T)
  compiler/
    Error.hpp
      An enum of compiler error types
      
    Node.cpp
      A Node in the graph is of type
        IO | Type | Module | Monad | Interface
        Node defines functions that the graph can use such as
          is_constructible_from(Type)
          is_convertible_to(Type)
    Graph.cpp
      Keeps a representation of
        Modules (exports)
          Scopes
            Interfaces
            Types
              Subtypes
      Accepts expressions as input
        -> Result<Node, Error>

    Template.cpp
      Defines the generative capacities of a template
    
  Module.cpp
    Properties of a module
  Monad.cpp
    Defines the properties of a monad and how to construct them
  Interface.cpp
    Defines what an Interface is
  Type.cpp
    Defines what a Type is and how to construct one
  IO.cpp
    Defines the properties of an IO object




  main.cpp