#pragma once

#include "xi/ext/scope.h"
#include "xi/ext/string.h"
#include "xi/ext/variant.h"
#include "xi/util/ownership.h"

namespace xi {
inline namespace ext {

  class optional_panic : public std::logic_error {
  public:
    optional_panic(string const &method)
        : logic_error("Trying to call " + method + " on a none value.") {
    }
  };

  template < class T >
  class optional;

  namespace detail {
    template < class T >
    struct optional_of {
      using type = optional< T >;
    };
    template < class T >
    struct optional_of< optional< T > > {
      using type = optional< T >;
    };
  }

  template < class T >
  using opt = typename detail::optional_of< T >::type;

  static constexpr struct none_t {
  } none{};

  template < class T >
  class optional final {
    using variant_t = variant< none_t, T >;
    variant_t _value = none;

  public:
    optional() = default;
    optional(optional< T > const &) = default;
    optional(optional< T > &&) = default;
    optional &operator=(optional< T > const &) = default;
    optional &operator=(optional< T > &&) = default;
    explicit optional(T value) : _value(move(value)) {
    }
    optional(none_t) : _value(none) {
    }

    bool is_none() const {
      return (nullptr != ext::get< none_t >(&_value));
    }
    bool is_some() const {
      return (!is_none());
    }

    auto as_mut();
    auto as_ref();

    T unwrap();
    T unwrap_or(T &&def);
    template < class G >
    T unwrap_or(G &&generator);

    template < class F, XI_UNLESS_DECL(is_same< void, result_of_t< F(T &&)> >)>
    auto map(F &&f) -> opt< result_of_t< F(T &&)> >;
    template < class F, XI_REQUIRE_DECL(is_same< void, result_of_t< F(T &&)> >)>
    void map(F &&f);
    template < class F, class U,
               XI_REQUIRE_DECL(is_same< U, result_of_t< F(T &&)> >)>
    auto map_or(U &&def, F &&f) -> U;
    template <
        class F, class G,
        XI_REQUIRE_DECL(is_same< result_of_t< G() >, result_of_t< F(T &&)> >)>
    auto map_or(G &&generator, F &&f) -> result_of_t< F(T &&)>;

    template < class U >
    auto and_(optional< U > &&optb) -> opt< U >;

    bool operator==(optional< T > const &other) const;
    bool operator!=(optional< T > const &other) const;
    bool operator<(optional< T > const &other) const;
    bool operator>(optional< T > const &other) const;

    operator bool() const;

  private:
    void panic_if_none(const char *method);
    void reset();
    T extract_value();
  };

  namespace detail {
    template < class T >
    struct _some {
      static opt< T > apply(T &&t) {
        return optional< T >(forward< T >(t));
      }
    };
    template < class T >
    struct _some< optional< T > > {
      static opt< T > apply(optional< T > &&t) {
        return move(t);
      }
    };
  }

  template < class T >
  opt< T > some(T t) {
    return detail::_some< T >::apply(move(t));
  }

  template < class T >
  auto optional< T >::as_mut() {
    if (is_none()) {
      return optional< mut< T > >(none);
    }
    return optional< mut< T > >(edit(ext::get< T >(_value)));
  }

  template < class T >
  auto optional< T >::as_ref() {
    if (is_none()) {
      return optional< ref< T > >(none);
    }
    return optional< ref< T > >(val(ext::get< T >(_value)));
  }

  template < class T >
  T optional< T >::unwrap() {
    panic_if_none("unwrap");
    return move(extract_value());
  }

  template < class T >
  T optional< T >::unwrap_or(T &&def) {
    if (is_none()) {
      return forward< T >(def);
    }
    return move(extract_value());
  }

  template < class T >
  template < class G >
  T optional< T >::unwrap_or(G &&generator) {
    if (is_none()) {
      return generator();
    }
    return move(extract_value());
  }

  template < class T >
  template < class F, XI_UNLESS(is_same< void, result_of_t< F(T &&)> >)>
  auto optional< T >::map(F &&f) -> opt< result_of_t< F(T &&)> > {
    if (is_none()) {
      return none;
    }
    return some(f(extract_value()));
  }

  template < class T >
  template < class F, XI_REQUIRE(is_same< void, result_of_t< F(T &&)> >)>
  void optional< T >::map(F &&f) {
    if (is_none()) {
      return;
    }
    return f(extract_value());
  }

  template < class T >
  template < class F, class U, XI_REQUIRE(is_same< U, result_of_t< F(T &&)> >)>
  auto optional< T >::map_or(U &&def, F &&f) -> U {
    if (is_none()) {
      return forward< result_of_t< F(T && ) > >(def);
    }
    return f(extract_value());
  }

  template < class T >
  template < class F, class G,
             XI_REQUIRE(is_same< result_of_t< G() >, result_of_t< F(T &&)> >)>
  auto optional< T >::map_or(G &&generator, F &&f) -> result_of_t< F(T &&)> {
    if (is_none()) {
      return generator();
    }
    return f(extract_value());
  }

  template < class T >
  template < class U >
  auto optional< T >::and_(optional< U > &&optb) -> opt< U > {
    if (is_none()) {
      return none;
    }
    return move(optb);
  }
  /// OPERARTORS
  template < class T >
  bool optional< T >::operator==(optional< T > const &other) const {
    if (is_none()) {
      return other.is_none();
    }
    return other.is_none() ? false : ext::get< T >(_value) ==
                                         ext::get< T >(other._value);
  }
  template < class T >
  bool optional< T >::operator!=(optional< T > const &other) const {
    return !(*this == other);
  }
  template < class T >
  bool optional< T >::operator<(optional< T > const &other) const {
    if (is_none() || other.is_none()) {
      return false;
    }
    return ext::get< T >(_value) < ext::get< T >(other._value);
  }
  template < class T >
  bool optional< T >::operator>(optional< T > const &other) const {
    return other < *this;
  }
  template < class T >
  optional< T >::operator bool() const {
    return is_some();
  }

  /// PRIVATE METHODS
  template < class T >
  void optional< T >::panic_if_none(const char *method) {
    if (is_none()) {
      throw optional_panic(method);
    }
  }
  template < class T >
  void optional< T >::reset() {
    // A hack to reassign value to variant with a reference type.
    _value.~variant();
    new (&_value) variant_t(none);
  }
  template < class T >
  T optional< T >::extract_value() {
    XI_SCOPE(exit) {
      reset();
    };
    return move(ext::get< T >(_value));
  }

} // inline namespace ext
} // namespace xi
