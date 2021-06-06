#pragma once

#include <cstddef>
#include <vector>
#include <unordered_map>
#include <memory>
#include <tuple>

#include "./defs.hpp"

namespace elk {
namespace compiler {
  using namespace std;

  using TypeID = std::size_t;
  static TypeID _type_idx { 0 };
  using FuncID = std::size_t;
  static FuncID _func_idx = { 0 };

  using TypePointer = shared_ptr<Type>;

  struct Graph { 
    vector<TypePointer> types;
    
    TypePointer
    insert_type(TypeDef&& def) {
      auto id = ++_type_idx;
      auto defines_constructor = def.constructors && def.constructors.value().size() > 0;

      types.push_back(make_shared<Type>(Type {
        .id = id,
        .name = def.name,
        .of = def.of
      }));

      return types.back();
    }

    vector<TypePointer>
    siblings_of(TypePointer child) const {
      if (!child->of) return {};
      auto desc = descendants_of(child->of.value());
      // c++20: ranges
      desc.erase(
        std::remove_if(
          desc.begin(), 
          desc.end(), 
          [&child](const TypePointer& c) {
            return child->id == c->id;
          }
        )
      );
      return desc;
    }
    
    vector<TypePointer>
    descendants_of(TypePointer parent) const {
      vector<TypePointer> vec {};

      // c++20: ranges
      std::copy_if(types.cbegin(), types.cend(), std::back_inserter(vec), [&parent](const TypePointer& item) {
        return item != parent && item->of == parent;
      });

      return vec;
    }

  };
}
}