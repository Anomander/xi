#pragma once

#include "xi/ext/Scope.h"
#include "xi/ext/String.h"
#include "xi/ext/Variant.h"
#include "xi/ext/Optional.h"
#include "xi/util/Ownership.h"

namespace xi {
inline namespace ext {

  class OptionPanic : public std::logic_error {
  public:
    OptionPanic(string const& method) : logic_error("Trying to call " + method + " on a None value.") {}
  };

  static constexpr struct None_t {
  } None{};

  template < class T >
  class Option final {
    using Variant_t = variant< None_t, T >;
    Variant_t _value = None;

  public:
    Option() = default;
    explicit Option(T value) : _value(move(value)) {}
    Option(None_t) : _value(None) {}

    bool isNone() const { return (nullptr != ext::get< None_t >(&_value)); }
    bool isSome() const { return (!isNone()); }

    auto asMut();
    auto asRef();

    T unwrap();
    T unwrapOr(T&& def);
    template < class Generator >
    T unwrapOr(Generator&& g);

    template < class Func >
    auto map(Func&& f) -> Option< result_of_t< Func(T&&)> >;
    template < class Func, class U, XI_REQUIRE_DECL(is_same< U, result_of_t< Func(T&&)> >)>
    auto mapOr(U&& def, Func&& f) -> Option< U >;
    template < class Func, class Generator,
               XI_REQUIRE_DECL(is_same< result_of_t< Generator() >, result_of_t< Func(T&&)> >)>
    auto mapOr(Generator&& g, Func&& f) -> Option< result_of_t< Func(T&&)> >;

    template < class U >
    auto and_(Option< U >&& optb) -> Option< U >;

    bool operator==(Option< T > const& other) const;
    bool operator!=(Option< T > const& other) const;
    bool operator<(Option< T > const& other) const;
    bool operator>(Option< T > const& other) const;

  private:
    void panicIfNone(const char* method);
    void reset();
    T extractValue();
  };

  template < class T >
  Option< T > Some(T t) {
    return Option< T >(move(t));
  }

  template < class T >
  auto Option< T >::asMut() {
    if (isNone()) {
      return Option< mut< T > >(None);
    }
    return Option< mut< T > >(edit(ext::get< T >(_value)));
  }

  template < class T >
  auto Option< T >::asRef() {
    if (isNone()) {
      return Option< ref< T > >(None);
    }
    return Option< ref< T > >(val(ext::get< T >(_value)));
  }

  template < class T >
  T Option< T >::unwrap() {
    panicIfNone("unwrap");
    return move(extractValue());
  }

  template < class T >
  T Option< T >::unwrapOr(T&& def) {
    if (isNone()) {
      return forward< T >(def);
    }
    return move(extractValue());
  }

  template < class T >
  template < class Generator >
  T Option< T >::unwrapOr(Generator&& g) {
    if (isNone()) {
      return g();
    }
    return move(extractValue());
  }

  template < class T >
  template < class Func >
  auto Option< T >::map(Func&& f) -> Option< result_of_t< Func(T&&)> > {
    if (isNone()) {
      return None;
    }
    return Some(f(extractValue()));
  }

  template < class T >
  template < class Func, class U, XI_REQUIRE_DECL(is_same< U, result_of_t< Func(T&&)> >)>
  auto Option< T >::mapOr(U&& def, Func&& f) -> Option< U > {
    if (isNone()) {
      return Some(forward< result_of_t< Func(T && ) > >(def));
    }
    return Some(f(extractValue()));
  }

  template < class T >
  template < class Func, class Generator,
             XI_REQUIRE_DECL(is_same< result_of_t< Generator() >, result_of_t< Func(T&&)> >)>
  auto Option< T >::mapOr(Generator&& g, Func&& f) -> Option< result_of_t< Func(T&&)> > {
    if (isNone()) {
      return Some(g());
    }
    return Some(f(extractValue()));
  }

  template < class T >
  template < class U >
  auto Option< T >::and_(Option< U >&& optb) -> Option< U > {
    if (isNone()) {
      return None;
    }
    return forward< Option< U > >(optb);
  }
  /// OPERARTORS
  template < class T >
  bool Option< T >::operator==(Option< T > const& other) const {
    if (isNone()) {
      return other.isNone();
    }
    return other.isNone() ? false : ext::get< T >(_value) == ext::get< T >(other._value);
  }
  template < class T >
  bool Option< T >::operator!=(Option< T > const& other) const {
    return !(*this == other);
  }
  template < class T >
  bool Option< T >::operator<(Option< T > const& other) const {
    if (isNone() || other.isNone()) {
      return false;
    }
    return ext::get< T >(_value) < ext::get< T >(other._value);
  }
  template < class T >
  bool Option< T >::operator>(Option< T > const& other) const {
    return other < *this;
  }

  /// PRIVATE METHODS
  template < class T >
  void Option< T >::panicIfNone(const char* method) {
    if (isNone()) {
      throw OptionPanic(method);
    }
  }
  template < class T >
  void Option< T >::reset() {
    // A hack to reassign value to variant with a reference type.
    _value.~variant();
    new (&_value) Variant_t(None);
  }
  template < class T >
  T Option< T >::extractValue() {
    XI_SCOPE(exit) { reset(); };
    return move(ext::get< T >(_value));
  }

} // inline namespace ext
} // namespace xi
