#pragma once

#include "ext/Common.h"
#include "ext/Meta.h"
#include "ext/Pointer.h"
#include "ext/TypeTraits.h"

namespace xi {
inline namespace util {

  namespace ownership {

    enum class SharedPolicy {
      kArc,
      kRc,
      kStd,
    };
    /// It might be worth taking the actual class
    /// and verifying that it doesn't declare a
    /// different ownership mode somewhere in its
    /// superclasses, but for now this should
    /// suffice
    template < SharedPolicy >
    struct Shared {};
    template <>
    struct Shared< SharedPolicy::kStd > : public ::std::enable_shared_from_this< Shared< SharedPolicy::kStd > > {};
    /// Pointing these to StdShared until either intrusive_ptr comes back
    /// or an alternative is found.
    template <>
    struct Shared< SharedPolicy::kRc > : public Shared< SharedPolicy::kStd > {};
    template <>
    struct Shared< SharedPolicy::kArc > : public Shared< SharedPolicy::kStd > {};
    struct Unique {};

    using StdShared = Shared< SharedPolicy::kStd >;
    using RcShared = Shared< SharedPolicy::kRc >;
    using ArcShared = Shared< SharedPolicy::kArc >;
  } // namespace ownership

  namespace detail {
    template < class T, ownership::SharedPolicy P >
    using EnableIfShared = meta::enable_if< is_base_of< ownership::Shared< P >, T > >;

    template < class T >
    using EnableIfUnique = meta::enable_if< is_base_of< ownership::Unique, T > >;

    template < class T >
    struct ensureNotConst {
      using type = remove_reference_t< remove_pointer_t< T > >;
      static_assert(!is_const< type >::value, "T must not be const");
    };
    template < class T >
    using ensureNotConst_t = typename ensureNotConst< T >::type;

    template < class T, class... >
    struct own {
      using type = remove_pointer_t< remove_reference_t< T > >;
    };
    template < class T, class... >
    struct ref {
      using type = add_lvalue_reference_t< add_const_t< remove_reference_t< T > > >;
    };
    template < class T, class... >
    struct mut {
      using type = add_pointer_t< ensureNotConst_t< T > >;
    };

    template < class T >
    struct ref< unique_ptr< T > > : ref< T > {};
    template < class T >
    struct ref< shared_ptr< T > > : ref< T > {};

    template < class T >
    struct mut< unique_ptr< T > > : mut< T > {};
    template < class T >
    struct mut< shared_ptr< T > > : mut< T > {};

    template < class T >
    struct own< T, EnableIfShared< T, ownership::SharedPolicy::kStd > > {
      using type = shared_ptr< T >;
    };
    template < class T >
    struct own< T, EnableIfUnique< T > > {
      using type = unique_ptr< T >;
    };

    template < class T, class... >
    struct Maker {
      template < class... Args >
      static auto make(Args&&... args) {
        return T{::std::forward< Args >(args)...};
      }
    };
    template < class T >
    struct Maker< T, EnableIfShared< T, ownership::SharedPolicy::kStd > > {
      template < class... Args >
      static auto make(Args&&... args) {
        return make_shared< T >(::std::forward< Args >(args)...);
      }
    };
    template < class T >
    struct Maker< T, EnableIfUnique< T > > {
      template < class... Args >
      static auto make(Args&&... args) {
        return make_unique< T >(::std::forward< Args >(args)...);
      }
    };

    template < class T, XI_REQUIRE_DECL(is_base_of< ownership::Shared< ownership::SharedPolicy::kStd >, T >)>
    auto share(T* obj) {
      return std::shared_ptr< T >(obj->shared_from_this(), reinterpret_cast< T* >(obj));
    }
  } // namespace detail

  template < class T >
  auto share(T* t) {
    return detail::share(t);
  }

  template < class T >
  using own = typename detail::own< T, meta::enabled >::type;
  template < class T >
  using ref = typename detail::ref< T, meta::enabled >::type;
  template < class T >
  using mut = typename detail::mut< T, meta::enabled >::type;

  template < class T, class... Args >
  own< T > make(Args&&... args) {
    return detail::Maker< T, meta::enabled >::make(::std::forward< Args >(args)...);
  }

  template < class T >
  inline own< T > copy(T&& t) {
    return forward< T >(t);
  }
  template < class T >
  inline ref< T > cref(T&& t) {
    return ref< T >(t);
  }
  template < class T >
  inline ref< T > cref(shared_ptr< T > t) {
    return ref< T >(*t);
  }
  template < class T >
  inline ref< T > cref(unique_ptr< T >& t) {
    return ref< T >(*t);
  }
  template < class T >
  inline ref< T > cref(unique_ptr< T > const& t) {
    return ref< T >(*t);
  }
  template < class T >
  inline mut< T > edit(T&& t) {
    return mut< T >(addressOf(t));
  }
  template < class T >
  inline mut< T > edit(shared_ptr< T > t) {
    return mut< T >(t.get());
  }
  template < class T >
  inline mut< T > edit(unique_ptr< T >& t) {
    return mut< T >(t.get());
  }
  template < class T >
  inline mut< T > edit(T const& t) = delete;
  template < class T >
  inline mut< T > edit(unique_ptr< T > const& t) = delete;

  template < class T >
  inline void release(unique_ptr< T > t) { /* no-op */
  }
  template < class T >
  inline void release(shared_ptr< T > t) { /* no-op */
  }

  template < class T >
  inline decltype(auto) val(T&& t) {
    return t;
  }
  template < class T >
  inline decltype(auto) val(T* t) {
    return *t;
  }
  template < class T >
  inline decltype(auto) val(shared_ptr< T > const& t) {
    return *t;
  }
  template < class T >
  inline decltype(auto) val(unique_ptr< T > const& t) {
    return *t;
  }
  template < class T >
  inline decltype(auto) val(unique_ptr< T >& t) {
    return *t;
  }

} // inline namespace util
} // namespace xi
