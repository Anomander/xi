#pragma once

#include <cstring>
#include <string>

#include <boost/utility/string_ref.hpp>

namespace xi {
inline namespace ext {

  using ::std::string;
  using ::std::to_string;

  template < typename char_t, typename traits >
  class basic_string_ref : public ::boost::basic_string_ref< char_t, traits > {
    using base = ::boost::basic_string_ref< char_t, traits >;

  public:
    constexpr basic_string_ref() : base() {
    }

    constexpr basic_string_ref(basic_string_ref const &rhs) : base(rhs) {
    }

    using base::operator=;

    template < size_t N >
    basic_string_ref(const char_t (&str)[N]) : basic_string_ref(str, N - 1) {
    }

    constexpr basic_string_ref(const char_t *str, size_t len) : base(str, len) {
    }

    template < typename allocator >
    basic_string_ref(const std::basic_string< char_t, traits, allocator > &str)
        : base(str) {
    }

    /// explicitly disallow construction from expiring string
    template < typename allocator >
    basic_string_ref(std::basic_string< char_t, traits, allocator > &&str) =
        delete;

  private:
    /// hide all constructors
    using base::basic_string_ref;
  };

  template < typename char_t, typename traits >
  inline auto to_string(basic_string_ref< char_t, traits > const &ref) {
    return ref.to_string();
  }

  using string_ref = basic_string_ref< char, ::std::char_traits< char > >;

} // inline namespace ext
} // namespace xi
