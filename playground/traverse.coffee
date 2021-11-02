# This is not CoffeeScript but pseudo-ish code that VSCode identified as CS
Memory =
  construct: (id: ConstructorID, args: ArgumentList) -> ValueID
  
TypeRegistry =
  register: (Type, Constructors) -> TypeID
    # Generate type_id
    # Store constructors for this type in Memory
    # Return type_id

  # Find a constructor for TypeID that can be called with ArgumentList
  find_constructor: (for: TypeID, args: ArgumentList) -> Optional<ConstructorID>
    # For each constructor of for_type_id
    # Match their ParameterList with the TypeRegistry of ArgumentList
    

Eval = (Scope: scope)
  find_type: (type_name: String) -> Optional<Type>
  find_type: (type_name: TypeID) -> Optional<Type>



Traverse
  match AstNode
    CallConstructor (constructor: ConstructorID, args: ArgumentList) ->
      values = Eval.args -> ValueList
      Memory.construct(constructor_id, args)

    CallConstructor (type: Type, args: ArgumentList) ->
      constructor_id = TypeRegistry.find_constructor(type.id, ArgumentList)
      Traverse(CallConstructor(constructor_id, args))

    CallConstructor (type: TypeRef, args: ArgumentList) ->
      type = Eval.find_type(TypeRef.symbol)
      Traverse(CallConstructor(type, args))

