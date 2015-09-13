#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace async {
  template < class T >
  struct LocalStorage {
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

  template < class T >
  static T *tryLocal() {
    return LocalStorage< T >::get();
  }
  template < class T >
  static T &local() {
    auto *ptr = tryLocal< T >();
    assert(nullptr != ptr);
    return *ptr;
  }
  template < class T >
  static void setLocal(T &t) {
    return LocalStorage< T >::set(t);
  }
  template < class T >
  static void resetLocal() {
    return LocalStorage< T >::reset();
  }
}
}
