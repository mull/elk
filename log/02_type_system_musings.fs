(*  There's not really any relation to F#, though I find its syntax quite nice
    The "Scalar" values can't be expressed in the type system, I guess. Hm. But
    their declaration maybe
*)
                            (*  Decl::Type {.name = "Int", .of = Type::Wrap<int, int> } 
                                Implicit:
                                  construct<Int>(Int) -> Int      // Int(Int) -> Int
                                  destruct<Int>(Int) -> Int       // Int~     -> Int
                            *)

(*
  The following scalars are provided. They can only be represented as abstract types
  in our type system, but there is a backing like above.
*)
type Int
type Str


type RegistrationShape      (*  Decl::Type {.name = "R...S...", .of = Type::Void }
                                Impossible to manifest, but possible to use in type equations
                            *) 

type RS = RegistrationShape (*  Decl::Type {.name = "RS", .of = Type::Alias<RegistrationShape>} 
                                Implicit:
                            *)

type HeightCm = Int         (*  Decl::Type {.name = "HeightCm", .of = Type::Alias<Int>}
                                Implicit:
                                  construct<HeightCm>(Int)
                            *)


type PK = Int | Str         (*  Decl::Type { .name = "PK", .of = Type::Sum<Pointer<Int>, Pointer<Str> }
                                Implicit:
                                  construct<ID>(Int)
                                  construct<ID>(Str)
                                  construct<ID>(Int | Str)
                                  destruct<ID>() -> Str
                                  destruct<ID>() -> Int
                                  destruct<ID>() -> Int | Str
                            *)

type Money = (Int, Int)     (*  Decl::Type {.name = "Money", .of = Type::Product<Pointer<Int>, Pointer<Int>>}
                                Implicit:
                                  construct<Money>(Int, Int) -> Money
                                  construct<Money>(Int) -> Partial<Money>(Int) -> Money
                                  destruct<Money>() -> (Int, Int)
                            *)

(*
  The Record type
  Merely hijacking the class keyword for syntax highlighting :')
  I would prefer something like "shape" or "record"
*)
class RegistrationShape     
  FirstName = Str           
  LastName = Str            
  Year                      
  Month                     
  Date                      

(*
  Abstract Record
*)               
class RegistrationShape             

(*
  Let's deconstruct.
  Opening up a Record means we introduce a new "namespace" (called RegistrationShape)
  and can define things in there.
*)
class RegistrationShape     (*  Decl::Type {.name = "RegistrationShape", .of = Type::Record } 
                                Unless we add some fields this remains an abstract record that cannot
                                be manifested. But just like abstract types, we can use it in type expressions
                             *)
  FirstName = Str           (*  Decl::Type {.name = "RegistrationShape::FirstName", .of = Pointer<String> }
                                Implicit:
                                  construct<RegistrationShape::FirstName>(RegistrationShape::FirstName)
                                  construct<RegistrationShape::FirstName>(Str)
                                  construct<RegistrationShape::FirstName(RegistrationShape)
                                  destruct<RegistrationShape::FirstName> -> Str
                             *)
  Year                      (*  Decl::Type {.name = "RegistrationShape::Year", .of = Types::Void }
                                An abstract symbol RegistrationShape::Year
                                This means the Record is still abstract
                             *)

