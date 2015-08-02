#pragma once

#include "util/Ownership.h"

namespace xi {
inline namespace util {

  template < class T >
  own< T > DefaultInitializer() {
    return make< T >();
  }

  template < class T >
  struct DefaultInitializable {
    using Initializer = function< own< T >() >;

    DefaultInitializable(Initializer init) : _initializer(move(init)) {
    }

    DefaultInitializable() : _initializer(&DefaultInitializer< T >) {
    }

    mut< T > get() {
      if (!_initialized) {
        _value = _initializer();
        _initialized = true;
      }
      return edit(_value);
    }

    void set(own< T > value) {
      _value = move(value);
      _initialized = true;
    }

    T* operator->() {
      return get();
    }

  private:
    bool _initialized = false;
    Initializer _initializer;
    own< T > _value;
  };

} // inline namespace util
} // namespace xi
