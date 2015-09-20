#pragma once

#include "xi/ext/Common.h"
#include "xi/ext/Meta.h"
#include "xi/ext/Pointer.h"
#include "xi/ext/Test.h"
#include "xi/ext/TypeTraits.h"
#include "xi/ext/Require.h"

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

    template < class T, class... Args >
    struct ref< unique_ptr< T >, Args... > : ref< T, Args... > {};
    template < class T, class... Args >
    struct ref< shared_ptr< T >, Args... > : ref< T, Args... > {};
    template < class T, class... Args >
    struct ref< T*, Args... > : ref< T, Args... > {};

    template < class T, class... Args >
    struct mut< unique_ptr< T >, Args... > : mut< T, Args... > {};
    template < class T, class... Args >
    struct mut< shared_ptr< T >, Args... > : mut< T, Args... > {};

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
    template < class T, XI_REQUIRE_DECL(is_base_of< ownership::Shared< ownership::SharedPolicy::kStd >, T >)>
    auto isShared(T* obj) {
      return obj->shared_from_this().use_count() > 2;
    }
  } // namespace detail

  template < class T >
  auto share(T* t) {
    return detail::share(t);
  }

  template < class T >
  shared_ptr< T > share(shared_ptr< T > const& t) {
    return t;
  }

  template < class T >
  bool isShared(T* t) {
    return detail::isShared(t);
  }

  template < class T >
  bool isShared(shared_ptr< T > const& t) {
    return t.use_count() > 1;
  }

  template < class T, class Deleter >
  bool isShared(unique_ptr< T, Deleter > const& t) {
    return false;
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
  inline mut< T > edit(T& t) {
    return &(t);
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
  inline decltype(auto) val(T const& t) {
    return t;
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

  namespace _static_test {

    class StdShared : public virtual ownership::StdShared {};
    class Unique : public virtual ownership::Unique {};

    STATIC_ASSERT_TEST(is_same< own< StdShared >, shared_ptr< StdShared > >);
    STATIC_ASSERT_TEST(is_same< own< Unique >, unique_ptr< Unique > >);
    STATIC_ASSERT_TEST(is_same< mut< StdShared >, StdShared* >);
    STATIC_ASSERT_TEST(is_same< mut< own< StdShared > >, StdShared* >);
    STATIC_ASSERT_TEST(is_same< mut< mut< StdShared > >, StdShared* >);
    STATIC_ASSERT_TEST(is_same< mut< Unique >, Unique* >);
    STATIC_ASSERT_TEST(is_same< mut< own< Unique > >, Unique* >);
    STATIC_ASSERT_TEST(is_same< mut< mut< Unique > >, Unique* >);
    STATIC_ASSERT_TEST(is_same< ref< StdShared >, StdShared const& >);
    STATIC_ASSERT_TEST(is_same< ref< own< StdShared > >, StdShared const& >);
    STATIC_ASSERT_TEST(is_same< ref< ref< StdShared > >, StdShared const& >);
    STATIC_ASSERT_TEST(is_same< ref< mut< StdShared > >, StdShared const& >);
    STATIC_ASSERT_TEST(is_same< ref< Unique >, Unique const& >);
    STATIC_ASSERT_TEST(is_same< ref< own< Unique > >, Unique const& >);
    STATIC_ASSERT_TEST(is_same< ref< ref< Unique > >, Unique const& >);
    STATIC_ASSERT_TEST(is_same< ref< mut< Unique > >, Unique const& >);

    STATIC_ASSERT_TEST(is_same< decltype(make< StdShared >()), shared_ptr< StdShared > >);
    STATIC_ASSERT_TEST(is_same< decltype(make< Unique >()), unique_ptr< Unique > >);

    STATIC_ASSERT_TEST(is_same< decltype(val(make< StdShared >())), StdShared& >);
    STATIC_ASSERT_TEST(is_same< decltype(val(make< Unique >())), Unique& >);

    STATIC_ASSERT_TEST(is_same< decltype(edit(make< StdShared >())), StdShared* >);
    STATIC_ASSERT_TEST(is_same< decltype(edit(declval< unique_ptr< Unique >& >())), Unique* >);

    STATIC_ASSERT_TEST(is_same< decltype(share(make< StdShared >())), shared_ptr< StdShared > >);
  }

} // inline namespace util
} // namespace xi
