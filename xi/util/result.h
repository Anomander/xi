#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/error.h"
#include "xi/ext/require.h"

namespace xi {
inline namespace prelude {
  class result_panic : public std::runtime_error {
  public:
    result_panic(int e)
        : runtime_error("Trying to get value out of an erroneous result (" +
                        to_string(e) + ").") {
    }
    result_panic(string err)
        : runtime_error("Trying to get value out of an erroneous result (" +
                        err + ").") {
    }
  };

  template < class T >
  class result;

  namespace detail {
    template < class T >
    struct result_ {
      using type = result< T >;
    };

    template < class T >
    struct result_< result< T > > {
      using type = result< T >;
    };

    template <>
    struct result_< void > {
      using type = result< void >;
    };

    template <>
    struct result_< meta::null > : result_< void > {};

    template < class T >
    using result_t = typename result_< T >::type;

    template < class T >
    using result_of_call_t = result_t< result_of_t< T > >;
  }

  template < class T = void >
  class[[gnu::warn_unused_result]] result {
    struct empty_t {};
    variant< T, error_code, empty_t > _value = empty_t{};

  public:
    XI_CLASS_DEFAULTS(result, ctor, copy, move);

    explicit result(T && t) : _value(move(t)) {
    }

    explicit result(T const &t) : _value(t) {
    }

    explicit result(error_code e) : _value(e) {
    }

    template < class E, XI_REQUIRE_DECL(is_error_code_enum< E >) >
    result(E e) : _value(error_code(e)) {
    }

    template < class U, XI_REQUIRE_DECL(is_convertible< U, T >) >
    result(result< U > && other) {
      if (other.is_ok()) {
        _value = other.template extract< U >();
      } else {
        _value = other.template extract< error_code >();
      }
    }

    ~result();

    bool is_ok() const;
    bool is_error() const;
    template < class E, XI_REQUIRE_DECL(is_error_code_enum< E >) >
    bool is_error(E e) const;

    T unwrap();
    T expect(string err);

    error_code to_error() const;
    opt< T > to_optional();

    bool operator==(error_code const &) const;
    bool operator!=(error_code const &) const;
    template < class U, XI_UNLESS_DECL(is_error_code_enum< U >) >
    bool operator!=(U const &other) const;
    template < class U, XI_UNLESS_DECL(is_error_code_enum< U >) >
    bool operator==(U const &other) const;
    template < class U, XI_UNLESS_DECL(is_error_code_enum< U >) >
    bool operator<(U const &other) const;
    template < class U, XI_UNLESS_DECL(is_error_code_enum< U >) >
    bool operator>(U const &other) const;

    template < class F,
               XI_UNLESS_DECL(is_same< void, result_of_t< F(T &&) > >) >
    auto then(F && f)->detail::result_of_call_t< F(T &&) >;
    template < class F,
               XI_REQUIRE_DECL(is_same< void, result_of_t< F(T &&) > >) >
    auto then(F && f)->detail::result_of_call_t< F(T &&) >;

  private:
    template < class >
    friend class result;

    bool is_empty() const;
    template < class U >
    U extract();
    T const &ref() const;
    void throw_if_error() const;
    void throw_if_error(string err) const;
  };

  template <>
  class result< void > : public result< meta::null > {
    template < class >
    friend class result;

  public:
    // These methods hide the inherited ones
    void unwrap();
    void expect(string err);
    opt< void > to_optional() = delete;

    using result< meta::null >::result;

    result(void) : result< meta::null >(meta::null{}) {
    }

    template < class U >
    result(result< U > &&other) {
      if (other.is_ok()) {
        other.template extract< U >();
        _value = meta::null{};
      } else {
        _value = other.template extract< error_code >();
      }
    }

    template < class F, XI_UNLESS_DECL(is_same< void, result_of_t< F() > >) >
    auto then(F &&f) -> detail::result_of_call_t< F() >;
    template < class F, XI_REQUIRE_DECL(is_same< void, result_of_t< F() > >) >
    auto then(F &&f) -> detail::result_of_call_t< F() >;
  };

  inline auto ok(void) {
    return result< void >();
  }

  template < class T >
  inline auto ok(T &&t) {
    return result< decay_t< T > >(forward< T >(t));
  }

  template < class T = void >
  inline auto err(error_code ec) {
    return result< T >(ec);
  }

  template < class T = void >
  inline auto result_from_errno() {
    return result< T >(error_code(errno, generic_category()));
  }

  template < class T >
  result< T >::~result() {
    // TODO: Come up with a better enforcement story
    // assert(!is_error());
    // throw_if_error();
  }

  template < class T >
  bool result< T >::is_ok() const {
    return nullptr != get< T >(&_value);
  }

  template < class T >
  bool result< T >::is_error() const {
    return nullptr != get< error_code >(&_value);
  }

  template < class T >
  template < class E, XI_REQUIRE(is_error_code_enum< E >) >
  bool result< T >::is_error(E e) const {
    return is_error() && to_error() == e;
  }

  template < class T >
  T result< T >::unwrap() {
    assert(is_ok());
    throw_if_error();
    return extract< T >();
  }

  template < class T >
  T result< T >::expect(string err) {
    assert(is_ok());
    throw_if_error(move(err));
    return extract< T >();
  }

  template < class T >
  error_code result< T >::to_error() const {
    return (is_error()) ? get< error_code >(_value) : error_code();
  }

  template < class T >
  opt< T > result< T >::to_optional() {
    return is_ok() ? some(unwrap()) : none;
  }

  template < class T >
  bool result< T >::operator==(error_code const &other) const {
    return to_error() == other;
  }

  template < class T >
  bool result< T >::operator!=(error_code const &other) const {
    return to_error() != other;
  }

  template < class T >
  template < class U, XI_UNLESS(is_error_code_enum< U >) >
  bool result< T >::operator!=(U const &other) const {
    return !(ref() == other);
  }

  template < class T >
  template < class U, XI_UNLESS(is_error_code_enum< U >) >
  bool result< T >::operator==(U const &other) const {
    return ref() == other;
  }

  template < class T >
  template < class U, XI_UNLESS(is_error_code_enum< U >) >
  bool result< T >::operator<(U const &other) const {
    return ref() < other;
  }

  template < class T >
  template < class U, XI_UNLESS(is_error_code_enum< U >) >
  bool result< T >::operator>(U const &other) const {
    return other < ref();
  }

  template < class T >
  template < class F, XI_UNLESS(is_same< void, result_of_t< F(T &&) > >) >
  auto result< T >::then(F &&f) -> detail::result_of_call_t< F(T &&) > {
    using return_t = detail::result_of_call_t< F(T &&) >;
    if (is_error()) {
      return return_t(extract< error_code >());
    }
    return return_t(f(extract< T >()));
  }

  template < class T >
  template < class F, XI_REQUIRE(is_same< void, result_of_t< F(T &&) > >) >
  auto result< T >::then(F &&f) -> detail::result_of_call_t< F(T &&) > {
    using return_t = detail::result_of_call_t< F(T &&) >;
    if (is_error()) {
      return return_t(extract< error_code >());
    }
    f(extract< T >());
    return return_t();
  }

  template < class F, XI_UNLESS(is_same< void, result_of_t< F() > >) >
  auto result< void >::then(F &&f) -> detail::result_of_call_t< F() > {
    using return_t = detail::result_of_call_t< F() >;
    if (is_error()) {
      return return_t(extract< error_code >());
    }
    return return_t(f());
  }

  template < class F, XI_REQUIRE(is_same< void, result_of_t< F() > >) >
  auto result< void >::then(F &&f) -> detail::result_of_call_t< F() > {
    using return_t = detail::result_of_call_t< F() >;
    if (is_error()) {
      return return_t(extract< error_code >());
    }
    return return_t(f());
  }

  template < class T >
  bool result< T >::is_empty() const {
    return nullptr != get< empty_t >(&_value);
  }

  template < class T >
  template < class U >
  U result< T >::extract() {
    XI_SCOPE(exit) {
      _value = empty_t{};
    };
    return move(get< U >(_value));
  }

  template < class T >
  T const &result< T >::ref() const {
    assert(is_ok());
    throw_if_error();
    return get< T >(_value);
  }

  template < class T >
  void result< T >::throw_if_error() const {
    if (is_error()) {
      throw system_error(get< error_code >(_value));
    }
  }
}
}
