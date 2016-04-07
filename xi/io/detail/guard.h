/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 * Copyright (C) 2016 Stan Pavlovski
 */
#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/tagged_ptr.h"

namespace xi {
namespace io {
  namespace detail {

    class guard final {
    public:
      struct impl;
      enum flags : u8 { USE_FREE = 1 };
      enum simple { SIMPLE_GUARD };

    private:
      // if bit 0 set, point to object to be freed directly.
      mutable tagged_ptr< impl, flags, 1 > _impl{nullptr};

    public:
      XI_CLASS_DEFAULTS(guard, ctor, no_copy);
      explicit guard(impl* i) : _impl(i) {
      }
      guard(simple, void* object) : _impl((impl*)object) {
        _impl.set_tag< USE_FREE >();
      }
      guard& operator=(guard&& x) noexcept;
      guard(guard&& x) noexcept : _impl(move(x._impl)) {
      }
      ~guard();

      guard share() const;
      bool empty() const;

      explicit operator bool() const {
        return bool(_impl);
      }

      /// Appends another guard to this guard.  When this guard is
      /// destroyed, both encapsulated actions will be carried out.
      void append(guard d);
    };

    struct guard::impl {
      mutable u64 refs = 1;
      guard next;
      impl(guard next) : next(move(next)) {
      }
      XI_CLASS_DEFAULTS(impl, virtual_dtor);
    };

    inline guard::~guard() {
      if (_impl.get_tag< USE_FREE >()) {
        ::free(_impl.get());
      } else if (_impl && --_impl->refs == 0) {
        delete _impl.get();
      }
    }

    inline guard& guard::operator=(guard&& x) noexcept {
      if (this != &x) {
        this->~guard();
        new (this) guard(move(x));
      }
      return *this;
    }

    template < typename F >
    struct func_guard_impl final : guard::impl {
      F func;
      func_guard_impl(guard next, F&& f) : impl(move(next)), func(move(f)) {
      }
      ~func_guard_impl() override {
        func();
      }
    };

    template < typename O >
    struct object_guard_impl final : guard::impl {
      O obj;
      object_guard_impl(guard next, O&& obj)
          : impl(move(next)), obj(move(obj)) {
      }
    };

    template < typename O >
    inline object_guard_impl< O >* make_object_guard_impl(guard next, O obj) {
      return new object_guard_impl< O >(move(next), move(obj));
    }

    struct free_guard_impl final : guard::impl {
      void* obj;
      free_guard_impl(void* obj) : impl(guard()), obj(obj) {
      }
      virtual ~free_guard_impl() override {
        free(obj);
      }
    };

    inline bool guard::empty() const {
      return !_impl;
    }

    inline guard guard::share() const {
      if (!_impl) {
        return guard();
      }
      if (_impl.get_tag< USE_FREE >() == USE_FREE) {
        _impl.reset(new free_guard_impl((void*)_impl.get()));
      }
      ++_impl->refs;
      return guard(_impl.get());
    }

    // Appends 'd' to the chain of guards. Avoids allocation if possible. For
    // performance reasons the current chain should be shorter and 'd' should be
    // longer.
    inline void guard::append(guard g) {
      if (g.empty()) {
        return;
      }
      auto i    = _impl.get();
      auto next = this;
      while (i) {
        assert(i != g._impl.get());
        if (next->_impl.get_tag< USE_FREE >() == USE_FREE) {
          i = new free_guard_impl((void*)_impl.get());
          next->_impl.reset(i);
        }
        if (i->refs != 1) {
          i = make_object_guard_impl(move(i->next), guard(i));
          next->_impl.reset(i);
        }
        next = &i->next;
        i    = next->_impl.get();
      }
      next->_impl = move(g._impl);
    }

    inline guard make_simple_guard(void* obj) {
      if (!obj) {
        return guard();
      }
      return guard(guard::SIMPLE_GUARD, obj);
    }

    template < typename T >
    inline guard make_object_guard(T&& obj) {
      return guard{make_object_guard_impl(guard(), move(obj))};
    }
  }
}
}
