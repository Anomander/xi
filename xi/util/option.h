#pragma once

#include "xi/ext/scope.h"
#include "xi/ext/string.h"
#include "xi/ext/variant.h"
#include "xi/util/ownership.h"

namespace xi {
inline namespace ext {

  class option_panic : public std::logic_error {
  public:
    option_panic(string const &method)
        : logic_error("Trying to call " + method + " on a none value.") {}
  };

  static constexpr struct none_t {
  } none{};

  template < class T > class option final {
    using variant_t = variant< none_t, T >;
    variant_t _value = none;

  public:
    option() = default;
    explicit option(T value) : _value(move(value)) {}
    option(none_t) : _value(none) {}

    bool is_none() const { return (nullptr != ext::get< none_t >(&_value)); }
    bool is_some() const { return (!is_none()); }

    auto as_mut();
    auto as_ref();

    T unwrap();
    T unwrap_or(T &&def);
    template < class generator > T unwrap_or(generator &&g);

    template < class func >
    auto map(func &&f) -> option< result_of_t< func(T &&)> >;
    template < class func, class U,
               XI_REQUIRE_DECL(is_same< U, result_of_t< func(T &&)> >)>
    auto map_or(U &&def, func &&f) -> option< U >;
    template < class func, class generator,
               XI_REQUIRE_DECL(is_same< result_of_t< generator() >,
                                        result_of_t< func(T &&)> >)>
    auto map_or(generator &&g, func &&f) -> option< result_of_t< func(T &&)> >;

    template < class U > auto and_(option< U > &&optb) -> option< U >;

    bool operator==(option< T > const &other) const;
    bool operator!=(option< T > const &other) const;
    bool operator<(option< T > const &other) const;
    bool operator>(option< T > const &other) const;

  private:
    void panic_if_none(const char *method);
    void reset();
    T extract_value();
  };

  template < class T > option< T > some(T t) { return option< T >(move(t)); }

  template < class T > auto option< T >::as_mut() {
    if (is_none()) { return option< mut< T > >(none); }
    return option< mut< T > >(edit(ext::get< T >(_value)));
  }

  template < class T > auto option< T >::as_ref() {
    if (is_none()) { return option< ref< T > >(none); }
    return option< ref< T > >(val(ext::get< T >(_value)));
  }

  template < class T > T option< T >::unwrap() {
    panic_if_none("unwrap");
    return move(extract_value());
  }

  template < class T > T option< T >::unwrap_or(T &&def) {
    if (is_none()) { return forward< T >(def); }
    return move(extract_value());
  }

  template < class T >
  template < class generator >
  T option< T >::unwrap_or(generator &&g) {
    if (is_none()) { return g(); }
    return move(extract_value());
  }

  template < class T >
  template < class func >
  auto option< T >::map(func &&f) -> option< result_of_t< func(T &&)> > {
    if (is_none()) { return none; }
    return some(f(extract_value()));
  }

  template < class T >
  template < class func, class U,
             XI_REQUIRE_DECL(is_same< U, result_of_t< func(T &&)> >)>
  auto option< T >::map_or(U &&def, func &&f) -> option< U > {
    if (is_none()) { return some(forward< result_of_t< func(T && ) > >(def)); }
    return some(f(extract_value()));
  }

  template < class T >
  template < class func, class generator,
             XI_REQUIRE_DECL(is_same< result_of_t< generator() >,
                                      result_of_t< func(T &&)> >)>
  auto option< T >::map_or(generator &&g, func &&f)
      -> option< result_of_t< func(T &&)> > {
    if (is_none()) { return some(g()); }
    return some(f(extract_value()));
  }

  template < class T >
  template < class U >
  auto option< T >::and_(option< U > &&optb) -> option< U > {
    if (is_none()) { return none; }
    return forward< option< U > >(optb);
  }
  /// OPERARTORS
  template < class T >
  bool option< T >::operator==(option< T > const &other) const {
    if (is_none()) { return other.is_none(); }
    return other.is_none() ? false : ext::get< T >(_value) ==
                                         ext::get< T >(other._value);
  }
  template < class T >
  bool option< T >::operator!=(option< T > const &other) const {
    return !(*this == other);
  }
  template < class T >
  bool option< T >::operator<(option< T > const &other) const {
    if (is_none() || other.is_none()) { return false; }
    return ext::get< T >(_value) < ext::get< T >(other._value);
  }
  template < class T >
  bool option< T >::operator>(option< T > const &other) const {
    return other < *this;
  }

  /// PRIVATE METHODS
  template < class T > void option< T >::panic_if_none(const char *method) {
    if (is_none()) { throw option_panic(method); }
  }
  template < class T > void option< T >::reset() {
    // A hack to reassign value to variant with a reference type.
    _value.~variant();
    new (&_value) variant_t(none);
  }
  template < class T > T option< T >::extract_value() {
    XI_SCOPE(exit) { reset(); };
    return move(ext::get< T >(_value));
  }

} // inline namespace ext
} // namespace xi
