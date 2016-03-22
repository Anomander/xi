#pragma once

#include "xi/ext/meta.h"
#include "xi/ext/type_traits.h"
#include "xi/util/optional.h"

/**
 * based on a stack_overflow answer by yakk
 * http://stackoverflow.com/a/20793156
 */
namespace xi {
inline namespace ext {
  // TODO: temporary location
  template < class T, class U >
  opt< shared_ptr< T > > consuming_dynamic_cast(shared_ptr< U > src) {
    auto cst = dynamic_pointer_cast< T >(move(src));
    if (cst) {
      return cst;
    } else {
      return none;
    }
  }

  template < class T, class U >
  opt< unique_ptr< T > > consuming_dynamic_cast(unique_ptr< U > src) {
    auto cst = dynamic_cast< T * >(src.get());
    if (cst) {
      src.release();
      return unique_ptr< T >(cst);
    } else {
      return none;
    }
  }

  template < typename T >
  struct fast_castable_node {
    virtual T *cast(T *overload_only = nullptr) {
      return nullptr;
    }
    virtual T const *cast(T *overload_only = nullptr) const {
      return nullptr;
    }

  protected:
    /// needs not be public, should not be accessible from outside
    ~fast_castable_node() noexcept = default;
  };

  template < class group >
  struct fast_castable;

  template <>
  /// empty cast group ends recursion
  struct fast_castable< meta::vector<> > {};

  template < class T, class... ts >
  struct fast_castable< meta::vector< T, ts... > >
      : fast_castable_node< T >, fast_castable< meta::vector< ts... > > {};

  template < class... ts >
  struct fast_castable_group : fast_castable< meta::vector< ts... > > {
    using fast_castable_group_types = meta::vector< ts... >;
  };

  template < typename T >
  T *fast_cast(fast_castable_node< T > *src) {
    return src->cast();
  }
  template < typename T >
  T const *fast_cast(fast_castable_node< T > const *src) {
    return src->cast();
  }
  template < typename T >
  shared_ptr< T > fast_cast(shared_ptr< fast_castable_node< T > > src) {
    auto *cast = src->cast();
    if (nullptr != cast) {
      return shared_ptr< T >(src, cast);
    }
    return nullptr;
  }
  template < typename T >
  auto consuming_fast_cast(shared_ptr< fast_castable_node< T > > src) {
    return fast_cast< T >(move(src));
  }
  template < typename T >
  unique_ptr< T > consuming_fast_cast(
      unique_ptr< fast_castable_node< T > > src) {
    auto *cast = src->cast();
    if (nullptr != cast) {
      return unique_ptr< T >(cast);
    }
    return nullptr;
  }

  template < typename dst, typename src >
  using is_fast_cast_allowed =
      meta::or_< is_base_of< dst, src >, is_same< dst, src > >;

  template < class D, class group, class types >
  struct fast_cast_group_member_impl;

  template < class D, class group >
  /// empty group ends the recursion
  /// this is where it inherits group
  /// and brings in all the pre-declared
  /// conversion operations
  struct fast_cast_group_member_impl< D, group, meta::vector<> > : group {};

  template < typename D, class group, typename T, typename... ts >
  /// the main implementation for each group member
  struct fast_cast_group_member_impl< D, group, meta::vector< T, ts... > >
      : fast_cast_group_member_impl< D, group, meta::vector< ts... > > {
  private:
    template < class S >
    static auto _do_cast(S *src, meta::true_type) {
      return static_cast< D * >(src);
    }
    template < class S >
    static auto _do_cast(S const *src, meta::true_type) {
      return static_cast< D const * >(src);
    }
    template < class src >
    static auto _do_cast(src *, meta::false_type) {
      return nullptr;
    }

  public:
    T *cast(T *overload_only = nullptr) override {
      return _do_cast(this, is_fast_cast_allowed< T, D >{});
    }
    T const *cast(T *overload_only = nullptr) const override {
      return _do_cast(this, is_fast_cast_allowed< T, D >{});
    }
  };

  template < class D, class group >
  struct fast_cast_group_member
      : fast_cast_group_member_impl<
            D,
            group,
            typename group::fast_castable_group_types > {
    static_assert(
        meta::contains< typename group::fast_castable_group_types, D >::value,
        "Declared type is not a member of the provided fast_castable_group. "
        "see "
        "the definition of the "
        "group and add the type to it.");
  };

} // inline namespace ext
} // namespace xi
