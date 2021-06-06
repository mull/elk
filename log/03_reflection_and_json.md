Given some syntax
```
record RegistrationInput {
  Str     firstName
  Str     lastName
}
```

And a type JSON

```
type JSONObject = Dictionary<string, JSON>
type JSONScalar = Str | Int | Float
type JSON = JSONObject | JSONScalar
```

Which implies for the simple cases the existence of:

```
via JSONScalar -> Optional<Str>
via JSONScalar -> Optional<Int>
via JSONScalar -> Optional<Float>
```

What to do about the JSONObject?

```
# Abstract
template<record T>
via JSON -> Optional<T>
```


```
# Concrete
template<record T>
via JSON -> Optional<T> =
  (T t) ->
    switch
    when JSONObject
      # map(T) is still compile time I guess
      let pairs = fields(T) * (Nothing)



      map pairs -> T # => Seq Optional<(Field, init<Field>)>
    else
      # Not much one can do about that
      Nothing<T>
```


