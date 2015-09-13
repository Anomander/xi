#pragma once

#include <tuple>

#include "xi/ext/Common.h"
#include "xi/ext/Meta.h"

namespace xi {
inline namespace ext {

  using ::std::tuple;
  using ::std::tuple_size;
  using ::std::make_tuple;

  struct Unpack {
  private:
    template < class Target, size_t Start, class Tuple >
    static decltype(auto) unpack(Target &&target, Tuple &&t) {
      return unpack< Target, Start >(meta::ulongGreater< std::tuple_size< Tuple >::value, Start >(),
                                     forward< Target >(target), forward< Tuple >(t));
    }
    template < class Target, size_t Start, class Tuple, class... Args >
    static decltype(auto) unpack(meta::TrueType, Target &&target, Tuple &&t, Args &&... args) {
      return unpack< Target, Start + 1 >(meta::ulongGreater< tuple_size< Tuple >::value, Start + 1 >(),
                                         forward< Target >(target), forward< Tuple >(t), forward< Args >(args)...,
                                         std::get< Start >(t));
    }
    template < class Target, size_t Start, class Tuple, class... Args >
    static decltype(auto) unpack(meta::FalseType, Target &&target, Tuple &&t, Args &&... args) {
      return target(forward< Args >(args)...);
    }

  public:
    template < class Target, size_t Start = 0, class... Args >
    static decltype(auto) apply(Target &&target, tuple< Args... > &&t) {
      return unpack< Target, Start >(forward< Target >(target), forward< tuple< Args... > >(t));
    }
  };
}
}
