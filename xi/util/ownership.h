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
    template < shared_policy > struct shared;
    template <>
    struct shared< shared_policy::kSTD >
        : public enable_shared_from_this< shared< shared_policy::kSTD > > {
      virtual ~shared() = default;
    };
    template <>
    struct shared< shared_policy::kRC >
        : public rc_counter< shared< shared_policy::kRC > > {
      virtual ~shared() = default;
    };
    template <>
    struct shared< shared_policy::kARC >
        : public arc_counter< shared< shared_policy::kRC > > {
      virtual ~shared() = default;
    };
    struct unique {};

    using std_shared = shared< shared_policy::kSTD >;
    using rc_shared = shared< shared_policy::kRC >;
    using arc_shared = shared< shared_policy::kARC >;
  } // namespace ownership

  namespace detail {
    template < class T, ownership::shared_policy P >
    struct is_shared_with_policy : is_base_of< ownership::shared< P >, T > {};
    template < class T >
    struct is_shared_with_policy< T, ownership::shared_policy::kRC >
        : meta::or_< is_base_of< ownership::rc_shared, T >,
                     is_base_of< rc_counter< T >, T > >::type {};
    template < class T >
    struct is_shared_with_policy< T, ownership::shared_policy::kARC >
        : meta::or_< is_base_of< ownership::arc_shared, T >,
                     is_base_of< arc_counter< T >, T > >::type {};

    template < class T, ownership::shared_policy P >
    using enable_if_shared = meta::enable_if< is_shared_with_policy< T, P > >;

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
    struct ref< intrusive_ptr< T >, A... > : ref< T, A... > {};
    template < class T, class... A >
    struct ref< T *, A... > : ref< T, A... > {};

    template < class T, class... A >
    struct mut< unique_ptr< T >, A... > : mut< T, A... > {};
    template < class T, class... A >
    struct mut< shared_ptr< T >, A... > : mut< T, A... > {};
    template < class T, class... A >
    struct mut< intrusive_ptr< T >, A... > : mut< T, A... > {};

    template < class T >
    struct own< T, enable_if_shared< T, ownership::shared_policy::kSTD > > {
      using type = shared_ptr< T >;
    };
    template < class T >
    struct own< T, enable_if_shared< T, ownership::shared_policy::kRC > > {
      using type = intrusive_ptr< T >;
    };
    template < class T >
    struct own< T, enable_if_shared< T, ownership::shared_policy::kARC > > {
      using type = intrusive_ptr< T >;
    };
    template < class T > struct own< T, enable_if_unique< T > > {
      using type = unique_ptr< T >;
    };

    template < class T, class... > struct maker {
      template < class... A > static auto make(A &&... args) {
        return T{forward< A >(args)...};
      }
    };
    template < class T >
    struct maker< T, enable_if_shared< T, ownership::shared_policy::kSTD > > {
      template < class... A > static auto make(A &&... args) {
        return make_shared< T >(forward< A >(args)...);
      }
    };
    template < class T >
    struct maker< T, enable_if_shared< T, ownership::shared_policy::kRC > > {
      template < class... A > static auto make(A &&... args) {
        return intrusive_ptr< T >(new T(forward< A >(args)...));
      }
    };
    template < class T >
    struct maker< T, enable_if_shared< T, ownership::shared_policy::kARC > > {
      template < class... A > static auto make(A &&... args) {
        return intrusive_ptr< T >(new T(forward< A >(args)...));
      }
    };
    template < class T > struct maker< T, enable_if_unique< T > > {
      template < class... A > static auto make(A &&... args) {
        return make_unique< T >(forward< A >(args)...);
      }
    };

    template < class T, XI_REQUIRE_DECL(is_shared_with_policy<
                            T, ownership::shared_policy::kSTD >)>
    auto share(T *obj) {
      return shared_ptr< T >(obj->shared_from_this(),
                             reinterpret_cast< T * >(obj));
    }
    template < class T, XI_REQUIRE_DECL(is_shared_with_policy<
                            T, ownership::shared_policy::kSTD >)>
    auto is_shared(T *obj) {
      return obj->shared_from_this().use_count() > 1;
    }

    template < class T, XI_REQUIRE_DECL(is_shared_with_policy<
                            T, ownership::shared_policy::kRC >)>
    auto share(T *obj) {
      return intrusive_ptr< T >(obj);
    }
    template < class T, XI_REQUIRE_DECL(is_shared_with_policy<
                            T, ownership::shared_policy::kRC >)>
    auto is_shared(T *obj) {
      return obj->use_count() > 1;
    }

    template < class T, XI_REQUIRE_DECL(is_shared_with_policy<
                            T, ownership::shared_policy::kARC >)>
    auto share(T *obj) {
      return intrusive_ptr< T >(obj);
    }
    template < class T, XI_REQUIRE_DECL(is_shared_with_policy<
                            T, ownership::shared_policy::kARC >)>
    auto is_shared(T *obj) {
      return obj->use_count() > 1;
    }
  } // namespace detail

  template < class T > auto share(T *t) { return detail::share(t); }

  template < class T > shared_ptr< T > share(shared_ptr< T > const &t) {
    return t;
  }

  template < class T > intrusive_ptr< T > share(intrusive_ptr< T > const &t) {
    return t;
  }

  template < class T > bool is_shared(T *t) { return detail::is_shared(t); }

  template < class T > bool is_shared(shared_ptr< T > const &t) {
    return t.use_count() > 1;
  }

  template < class T > bool is_shared(intrusive_ptr< T > const &t) {
    return t.use_count() > 1;
  }

  template < class T, class deleter >
  bool is_shared(unique_ptr< T, deleter > const &t) {
    return false;
  }

  template < class T > bool is_valid(T *t) { return t != nullptr; }

  template < class T > bool is_valid(T const &t) { return true; }

  template < class T > bool is_valid(shared_ptr< T > const &t) {
    return is_valid(t.get());
  }

  template < class T > bool is_valid(intrusive_ptr< T > const &t) {
    return is_valid(t.get());
  }

  template < class T, class Deleter >
  bool is_valid(unique_ptr< T, Deleter > const &t) {
    return is_valid(t.get());
  }

  template < class T >
  using own = typename detail::own< T, meta::enabled >::type;
  template < class T >
  using ref = typename detail::ref< T, meta::enabled >::type;
  template < class T >
  using mut = typename detail::mut< T, meta::enabled >::type;

  template < class T, class... A > own< T > make(A &&... args) {
    return detail::maker< T, meta::enabled >::make(forward< A >(args)...);
  }

  template < class T > inline mut< T > edit(T &t) { return &(t); }
  template < class T > inline mut< T > edit(shared_ptr< T > t) {
    return mut< T >(t.get());
  }
  template < class T > inline mut< T > edit(intrusive_ptr< T > t) {
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
  template < class T > inline void release(intrusive_ptr< T > t) { /* no-op */
  }

  template < class T > inline decltype(auto) val(T const &t) { return t; }
  template < class T > inline decltype(auto) val(shared_ptr< T > const &t) {
    return *t;
  }
  template < class T > inline decltype(auto) val(intrusive_ptr< T > const &t) {
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
    class rc_shared : public virtual ownership::rc_shared {};
    class arc_shared : public virtual ownership::arc_shared {};
    class rc_counted : public virtual rc_counter< rc_counted > {};
    class arc_counted : public virtual arc_counter< arc_counted > {};
    class unique : public virtual ownership::unique {};

    STATIC_ASSERT_TEST(is_same< own< std_shared >, shared_ptr< std_shared > >);
    STATIC_ASSERT_TEST(is_same< own< std_shared >, shared_ptr< std_shared > >);
    STATIC_ASSERT_TEST(is_same< own< rc_shared >, intrusive_ptr< rc_shared > >);
    STATIC_ASSERT_TEST(
        is_same< own< rc_counted >, intrusive_ptr< rc_counted > >);
    STATIC_ASSERT_TEST(
        is_same< own< arc_counted >, intrusive_ptr< arc_counted > >);
    STATIC_ASSERT_TEST(detail::is_shared_with_policy<
        std_shared, ownership::shared_policy::kSTD >);
    STATIC_ASSERT_TEST(detail::is_shared_with_policy<
        rc_shared, ownership::shared_policy::kRC >);
    STATIC_ASSERT_TEST(detail::is_shared_with_policy<
        rc_counted, ownership::shared_policy::kRC >);
    STATIC_ASSERT_TEST(detail::is_shared_with_policy<
        arc_shared, ownership::shared_policy::kARC >);
    STATIC_ASSERT_TEST(detail::is_shared_with_policy<
        arc_counted, ownership::shared_policy::kARC >);

    STATIC_ASSERT_TEST(
        is_same< own< arc_shared >, intrusive_ptr< arc_shared > >);
    STATIC_ASSERT_TEST(is_same< own< unique >, unique_ptr< unique > >);
    STATIC_ASSERT_TEST(is_same< mut< std_shared >, std_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< rc_shared >, rc_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< arc_shared >, arc_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< own< std_shared > >, std_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< mut< std_shared > >, std_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< own< rc_shared > >, rc_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< mut< rc_shared > >, rc_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< own< arc_shared > >, arc_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< mut< arc_shared > >, arc_shared * >);
    STATIC_ASSERT_TEST(is_same< mut< unique >, unique * >);
    STATIC_ASSERT_TEST(is_same< mut< own< unique > >, unique * >);
    STATIC_ASSERT_TEST(is_same< mut< mut< unique > >, unique * >);
    STATIC_ASSERT_TEST(is_same< ref< std_shared >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< own< std_shared > >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< ref< std_shared > >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< mut< std_shared > >, std_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< rc_shared >, rc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< own< rc_shared > >, rc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< ref< rc_shared > >, rc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< mut< rc_shared > >, rc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< arc_shared >, arc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< own< arc_shared > >, arc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< ref< arc_shared > >, arc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< mut< arc_shared > >, arc_shared const & >);
    STATIC_ASSERT_TEST(is_same< ref< unique >, unique const & >);
    STATIC_ASSERT_TEST(is_same< ref< own< unique > >, unique const & >);
    STATIC_ASSERT_TEST(is_same< ref< ref< unique > >, unique const & >);
    STATIC_ASSERT_TEST(is_same< ref< mut< unique > >, unique const & >);

    STATIC_ASSERT_TEST(
        is_same< decltype(make< std_shared >()), shared_ptr< std_shared > >);
    STATIC_ASSERT_TEST(
        is_same< decltype(make< rc_shared >()), intrusive_ptr< rc_shared > >);
    STATIC_ASSERT_TEST(
        is_same< decltype(make< arc_shared >()), intrusive_ptr< arc_shared > >);
    STATIC_ASSERT_TEST(
        is_same< decltype(make< rc_counted >()), intrusive_ptr< rc_counted > >);
    STATIC_ASSERT_TEST(is_same< decltype(make< arc_counted >()),
                                intrusive_ptr< arc_counted > >);
    STATIC_ASSERT_TEST(
        is_same< decltype(make< unique >()), unique_ptr< unique > >);

    STATIC_ASSERT_TEST(
        is_same< decltype(val(make< std_shared >())), std_shared & >);
    STATIC_ASSERT_TEST(
        is_same< decltype(val(make< rc_shared >())), rc_shared & >);
    STATIC_ASSERT_TEST(
        is_same< decltype(val(make< arc_shared >())), arc_shared & >);
    STATIC_ASSERT_TEST(is_same< decltype(val(make< unique >())), unique & >);

    STATIC_ASSERT_TEST(
        is_same< decltype(edit(make< std_shared >())), std_shared * >);
    STATIC_ASSERT_TEST(
        is_same< decltype(edit(make< rc_shared >())), rc_shared * >);
    STATIC_ASSERT_TEST(
        is_same< decltype(edit(make< arc_shared >())), arc_shared * >);
    STATIC_ASSERT_TEST(is_same<
        decltype(edit(declval< unique_ptr< unique > & >())), unique * >);

    STATIC_ASSERT_TEST(is_same< decltype(share(make< std_shared >())),
                                shared_ptr< std_shared > >);
    STATIC_ASSERT_TEST(is_same< decltype(share(make< rc_shared >())),
                                intrusive_ptr< rc_shared > >);
    STATIC_ASSERT_TEST(is_same< decltype(share(make< arc_shared >())),
                                intrusive_ptr< arc_shared > >);
  }

} // inline namespace util
} // namespace xi
