#pragma once

#include "ext/Meta.h"
#include "ext/TypeTraits.h"
#include "ext/Optional.h"

/**
 * Based on a StackOverflow answer by Yakk
 * http://stackoverflow.com/a/20793156
 */
namespace xi {
inline namespace ext {
  // TODO: Temporary location
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
    auto cst = dynamic_cast< T* >(src.get());
    if (cst) {
      src.release();
      return unique_ptr< T >(cst);
    } else {
      return none;
    }
  }

  template < typename T >
  struct FastCastableNode {
    virtual T* cast(T* overloadOnly = nullptr) {
      return nullptr;
    }
    virtual T const* cast(T* overloadOnly = nullptr) const {
      return nullptr;
    }

  protected:
    /// Needs not be public, should not be accessible from outside
    ~FastCastableNode() noexcept = default;
  };

  template < class Group >
  struct FastCastable;

  template <>
  /// Empty cast group ends recursion
  struct FastCastable< meta::vector<> > {};

  template < class T, class... Ts >
  struct FastCastable< meta::vector< T, Ts... > > : FastCastableNode< T >, FastCastable< meta::vector< Ts... > > {};

  template < class... Ts >
  struct FastCastableGroup : FastCastable< meta::vector< Ts... > > {
    using FastCastableGroupTypes = meta::vector< Ts... >;
  };

  template < typename T >
  T* fast_cast(FastCastableNode< T >* src) {
    return src->cast();
  }
  template < typename T >
  T const* fast_cast(FastCastableNode< T > const* src) {
    return src->cast();
  }
  template < typename T >
  shared_ptr< T > fast_cast(shared_ptr< FastCastableNode< T > > src) {
    auto* cast = src->cast();
    if (nullptr != cast) {
      return shared_ptr< T >(src, cast);
    }
    return nullptr;
  }
  template < typename T >
  auto consuming_fast_cast(shared_ptr< FastCastableNode< T > > src) {
    return fast_cast< T >(move(src));
  }
  template < typename T >
  unique_ptr< T > consuming_fast_cast(unique_ptr< FastCastableNode< T > > src) {
    auto* cast = src->cast();
    if (nullptr != cast) {
      return unique_ptr< T >(cast);
    }
    return nullptr;
  }

  template < typename Dst, typename Src >
  using IsFastCastAllowed = meta::or_< is_base_of< Dst, Src >, is_same< Dst, Src > >;

  template < class D, class Group, class Types >
  struct FastCastGroupMemberImpl;

  template < class D, class Group >
  /// Empty group ends the recursion
  /// This is where it inherits group
  /// and brings in all the pre-declared
  /// conversion operations
  struct FastCastGroupMemberImpl< D, Group, meta::vector<> > : Group {};

  template < typename D, class Group, typename T, typename... Ts >
  /// The main implementation for each group member
  struct FastCastGroupMemberImpl< D, Group, meta::vector< T, Ts... > >
      : FastCastGroupMemberImpl< D, Group, meta::vector< Ts... > > {
  private:
    template < class Src >
    static auto _doCast(Src* src, meta::TrueType) {
      return static_cast< D* >(src);
    }
    template < class Src >
    static auto _doCast(Src const* src, meta::TrueType) {
      return static_cast< D const* >(src);
    }
    template < class Src >
    static auto _doCast(Src*, meta::FalseType) {
      return nullptr;
    }

  public:
    T* cast(T* overloadOnly = nullptr) override {
      return _doCast(this, IsFastCastAllowed< T, D >{});
    }
    T const* cast(T* overloadOnly = nullptr) const override {
      return _doCast(this, IsFastCastAllowed< T, D >{});
    }
  };

  template < class D, class Group >
  struct FastCastGroupMember : FastCastGroupMemberImpl< D, Group, typename Group::FastCastableGroupTypes > {
    static_assert(meta::contains< typename Group::FastCastableGroupTypes, D >::value,
                  "Declared type is not a member of the provided FastCastableGroup. See "
                  "the definition of the "
                  "group and add the type to it.");
  };

} // inline namespace ext
} // namespace xi
