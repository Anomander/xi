#pragma once

#include "async/TaskQueue.h"

namespace xi {
namespace async2 {
  struct Context {
    template < class T >
    struct LocalStorage {
      static T &get() {
        if (!ptr()) {
          set(getDefault());
        }
        return *ptr();
      }
      static void set(T &t) {
        assert(ptr() == nullptr);
        ptr() = &t;
      }
      static void reset() {
        ptr() = nullptr;
      }

    private:
      static T *&ptr() {
        static thread_local T *PTR = nullptr;
        return PTR;
      }
      static T &getDefault() {
        static thread_local T OBJ;
        return OBJ;
      }
    };
    template < class T >
    static T &local() {
      return LocalStorage< T >::get();
    }
    template < class T >
    static void setLocal(T &t) {
      return LocalStorage< T >::set(t);
    }
    template < class T >
    static void resetLocal() {
      return LocalStorage< T >::reset();
    }

    static void initialize() {
    }

  private:
    Context() {
    }
    Context(Context const &) = delete;
    Context(Context &&) = delete;
  };

  template < class T >
  decltype(auto) local() {
    return Context::local< T >();
  }

  /// Override to always return the same Context object.
  template <>
  struct Context::LocalStorage< Context > {
    static Context &get() {
      static Context OBJ;
      return OBJ;
    }
  };
}
}
