/**
 * What are we trying to do:
 * 

type LinePath = (x: Int, y: Int)
* Is short for a function taking a (named) pair of Int and returns that pair of ints
type LinePath = (x: Int, y: Int) 
  (x, y) -> (x, y)

type LinePathAdder = 
  Accumulate<List<Int>>
    * Optional
    (list) -> append list
    (list item) -> append list item

type Logger =
  Accumulate<List<Str>>(list)

type Logger = 
  append<List<Str>>

type GraphLinePathAdder =
  append<List Int>(previous y) -> append (smoothly_from previous y) previous



let avg_of<Seq<Divisible T>>(seq) -> 
  let sum = sum_of(seq)
  let len = length_of(seq)
  divide sum len

 
IO<log: Logger, t: Translator, line: GraphLinePathAdder>
fn calc_node_value(node: GraphNode) {
@log Node
@line (node.x, node.y)
let neighborhood_average = avg_of (edges_of node) :length
let neighborhood_diff_average = avg_of (edges_of node) :diff_from_average

min_of 
  neighborhood_average
  neighborhood_diff_average
}
*/
#include <string>
#include <iostream>
#include <concepts>

template<typename T>
concept Contextual = requires(T a) {
  // { a() };
  { a };
};

template <class From, class To>
concept convertible_to =
  std::is_convertible_v<From, To> &&
  requires(std::add_rvalue_reference_t<From> (&f)()) {
    static_cast<To>(f());
  };

template<typename T>
concept Stringable = convertible_to<T, std::string>;

struct Logger {
  template<Stringable T>
  void operator()(const T item) {
    std::cout << item << "\n";
  }
};

static_assert(Contextual<Logger>);
// static_assert(Stringable<int>);

struct Node {
  int id;
  std::string name;
};

int main() {
  Logger log;

  Node node { .id = 1, .name = "Foo!" };
  // log(node.id);
  log(node.name);
}