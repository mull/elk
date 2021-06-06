If part of the goal is for as many things as possible to be compositions of some other things, then what are records? They ought (?) to build upon types somehow.

```
type MyRecord 
```

Seems like a "closed statement." There's a somewhat logical reason as to why Haskell seems to create functions in the same scope as the record.

At least from the POV of the type system, I think the following two statements should be equal:

```
type RegistrationInput
record RegistrationInput
```

And perhaps the answer down into the details is what the "backend implementation" of that type is.

```c++
auto Int = Type {
  .name = "Int",
  .container = ScalarContainer
};

auto SomeRecord = Type {
  .name = "SomeRecord",
  .container = RecordContainer
};
```

And the properties of those containers would be very different. Particularly reflecting on the Int type would not reveal much interesting about it, but for SomeRecord there could be quite a few things to discover.

Assuming that SomeRecord has two fields both of type `Optional<Str>`, then the difference might be something like:

```
reflect Int
{
  type: Concrete
  container: IntContainer
  constructors: [
    (Int) -> Int
  ]
}

reflect SomeRecord
{
  type: Concrete
  container: RecordContainer 
  constructors: [
    (firstName: Optional<Str>,  lastName: Optional<Str>)  -> SomeRecord
    # Which implies the existence of
    (firstName: Str,            lastName: Optional<Str>)  -> SomeRecord
    (firstName: Optional<Str>,  lastName: Str)            -> SomeRecord
    (firstName: Str,            lastName: Str)            -> SomeRecord
    # Because Optional<Str> defines Str -> Optional<Str>
  ]
  fields: [
    (Optional<Str>, "firstName")
    (Optional<Str>, "lastName")
  ]
}
```
