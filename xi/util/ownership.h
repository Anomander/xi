#pragma once

#include "xi/ext/common.h"
#include "xi/ext/meta.h"
#include "xi/ext/pointer.h"
#include "xi/ext/test.h"
#include "xi/ext/type_traits.h"
#include "xi/ext/require.h"

namespace xi {
inline namespace util {

  namespace ownership {

    enum class shared_policy {
      kARC,
      kRC,
      kSTD,
    };
    /// it might be worth taking the actual class
    /// and verifying that it doesn't declare a
    /// different ownership mode somewhere in its
    /// superclasses, but for now this should
    /// suffice
    template < shared_policy > struct shared {};
    template <>
    struct shared< shared_policy::kSTD >
        : public ::std::enable_shared_from_this<
              shared< shared_policy::kSTD > > {};
    /// pointing these to std_shared until either intrusive_ptr comes back
    /// or an alternative is found.
    template <>
    struct shared< shared_policy::kRC > : public shared< shared_policy::kSTD > {
    };
    template <>
    struct shared< shared_policy::kARC >
        : public shared< shared_policy::kSTD > {};
    struct unique {};

    using std_shared = shared< shared_policy::kSTD >;
    using rc_shared = shared< shared_policy::kRC >;
    using arc_shared = shared< shared_policy::kARC >;
  } // namespace ownership

  namespace detail {
    template < class T, ownership::shared_policy P >
    using enable_if_shared =
        meta::enable_if< is_base_of< ownership::shared< P >, T > >;

    template < class T >
    using enable_if_unique =
        meta::enable_if< is_base_of< ownership::unique, T > >;

    template < class T > struct ensure_not_const {
      using type = remove_reference_t< remove_pointer_t< T > >;
      static_assert(!is_const< type >::value, "T must not be const");
    };
    template < class T >
    using ensure_not_const_t = typename ensure_not_const< T >::type;

    template < class T, class... > struct own {
      using type = remove_pointer_t< remove_reference_t< T > >;
    };
    template < class T, class... > struct ref {
      using type =
          add_lvalue_reference_t< add_const_t< remove_reference_t< T > > >;
    };
    template < class T, class... > struct mut {
      using type = add_pointer_t< ensure_not_const_t< T > >;
    };

    template < class T, class... A >
    struct ref< unique_ptr< T >, A... > : ref< T, A... > {};
    template < class T, class... A >
    struct ref< shared_ptr< T >, A... > : ref< T, A... > {};
    template < class T, class... A >
    struct ref< T *, A... > : ref< T, A... > {};

    template < class T, class... A >
    struct mut< unique_ptr< T >, A... > : mut< T, A... > {};
    template < class T, class... A >
    struct mut< shared_ptr< T >, A... > : mut< T, A... > {};

    template < class T >
    struct own< T, enable_if_shared< T, ownership::shared_policy::kSTD > > {
      using type = shared_ptr< T >;
    };
    template < class T > struct own< T, enable_if_unique< T > > {
      using type = unique_ptr< T >;
    };

    template < class T, class... > struct maker {
      template < class... A > static auto make(A &&... args) {
        return T{::std::forward< A >(args)...};
      }
    };
    template < class T >
    struct maker< T, enable_if_shared< T, ownership::shared_policy::kSTD > > {
      template < class... A > static auto make(A &&... args) {
        return make_shared< T >(::std::forward< A >(args)...);
      }
    };
    template < class T > struct maker< T, enable_if_unique< T > > {
      template < class... A > static auto make(A &&... args) {
        return make_unique< T >(::std::forward< A >(args)...);
      }
    };

    template < class T,
               XI_REQUIRE_DECL(is_base_of<
                   ownership::shared< ownership::shared_policy::kSTD >, T >)>
    auto share(T *obj) {
      return std::shared_ptr< T >(obj->shared_from_this(),
                                  reinterpret_cast< T * >(obj));
    }
    template < class T,
               XI_REQUIRE_DECL(is_base_of<
                   ownership::shared< ownership::shared_policy::kSTD >, T >)>
    auto is_shared(T *obj) {
      return obj->shared_from_this().use_count() > 2;
    }
  } // namespace detail

  template < class T > auto share(T *t) { return detail::share(t); }

  template < class T > shared_ptr< T > share(shared_ptr< T > const &t) {
    return t;
  }

  template < class T > bool is_shared(T *t) { return detail::is_shared(t); }

  template < class T > bool is_shared(shared_ptr< T > const &t) {
    return t.use_count() > 1;
  }

  template < class T, class deleter >
  bool is_shared(unique_ptr< T, deleter > const &t) {
    return false;
  }

  template < class T >
  using own = typename detail::own< T, meta::enabled >::type;
  template < class T >
  using ref = typename detail::ref< T, meta::enabled >::type;
  template < class T >
  using mut = typename detail::mut< T, meta::enabled >::type;

  template < class T, class... A > own< T > make(A &&... args) {
    return detail::maker< T, meta::enabled >::make(
        ::std::forward< A >(args)...);
  }

  template < class T > inline mut< T > edit(T &t) { return &(t); }
  template < class T > inline mut< T > edit(shared_ptr< T > t) {
    return mut< T >(t.get());
  }
  template < class T > inline mut< T > edit(unique_ptr< T > &t) {
    return mut< T >(t.get());
  }
  template < class T > inline mut< T > edit(T const &t) = delete;
  template < class T > inline mut< T > edit(unique_ptr< T > const &t) = delete;

  template < class T > inline void release(unique_ptr< T > t) { /* no-op */
  }
  template < class T > inline void release(shared_ptr< T > t) { /* no-op */
  }

  template < class T > inline decltype(auto) val(T const &t) { return t; }
  template < class T > inline decltype(auto) val(shared_ptr< T > const &t) {
    return *t;
  }
  template < class T > inline decltype(auto) val(unique_ptr< T > const &t) {
    return *t;
  }
  template < class T > inline decltype(auto) val(unique_ptr< T > &t) {
    return *t;
  }

  namespace _static_test {

    class std_shared : public virtual ownership::std_shared {};
    class unique : public virtual ownership::unique {};

    STATIC_ASSERT_TEST(is_same< own< std_shared >, shared_ptr< std_shared > >);
    STATIC_ASSERT_TEST(is_same< own< unique >, unique_ptr< unique > >);
    STATIC_ASSERT_TEST(is_same< mut< std_shared >, std_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< own< std_shared > >, std_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< mut< std_shared > >, std_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< unique >, unique * >);
    STATIC_ASSERT_TEST(is_same< mut< own< unique > >, unique * >);
    STATIC_ASSERT_TEST(is_same< mut< mut< unique > >, unique * >);
    STATIC_ASSERT_TEST(is_same< ref< std_shared >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< own< std_shared > >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< ref< std_shared > >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< mut< std_shared > >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< unique >, unique const & >);
    STATIC_ASSERT_TEST(is_same< ref< own< unique > >, unique const & >);
    STATIC_ASSERT_TEST(is_same< ref< ref< unique > >, unique const & >);
    STATIC_ASSERT_TEST(is_same< ref< mut< unique > >, unique const & >);

    STATIC_ASSERT_TEST(
        is_same< decltype(make< std_shared >()), shared_ptr< std_shared > >);
    STATIC_ASSERT_TEST(
        is_same< decltype(make< unique >()), unique_ptr< unique > >);

    STATIC_ASSERT_TEST(
        is_same< decltype(val(make< std_shared >())), std_shared & >);
    STATIC_ASSERT_TEST(is_same< decltype(val(make< unique >())), unique & >);

    STATIC_ASSERT_TEST(
        is_same< decltype(edit(make< std_shared >())), std_shared * >);
    STATIC_ASSERT_TEST(is_same<
        decltype(edit(declval< unique_ptr< unique > & >())), unique * >);

    STATIC_ASSERT_TEST(is_same< decltype(share(make< std_shared >())),
                                shared_ptr< std_shared > >);
  }

} // inline namespace util
} // namespace xi
