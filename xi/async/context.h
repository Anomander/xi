#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace async {
  template < class T > struct local_storage {
    static T *get() { return ptr(); }
    static void set(T &t) {
      assert(ptr() == nullptr);
      ptr() = &t;
    }
    static void reset() {
      ptr() = nullptr;
      assert(ptr() == nullptr);
    }

  private:
    static T *&ptr() {
      static thread_local T *PTR = nullptr;
      return PTR;
    }
  };

  template < class T > static T *try_local() {
    return local_storage< T >::get();
  }
  template < class T > static T &local() {
    auto *ptr = try_local< T >();
    assert(nullptr != ptr);
    return *ptr;
  }
  template < class T > static void set_local(T &t) {
    return local_storage< T >::set(t);
  }
  template < class T > static void reset_local() {
    return local_storage< T >::reset();
  }
}
}
