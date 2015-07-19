#pragma once

#include <string>
#include <cstring>

#include <boost/format.hpp>
#include <boost/utility/string_ref.hpp>

namespace xi {
inline namespace ext {

using ::std::string;
using ::std::to_string;

using ::boost::format;
using ::boost::str;

template < typename CharT, typename Traits > class BasicStringRef : public ::boost::basic_string_ref< CharT, Traits > {
  using Base = ::boost::basic_string_ref< CharT, Traits >;

public:
  constexpr BasicStringRef() : Base() {}

  constexpr BasicStringRef(BasicStringRef const& rhs) : Base(rhs) {}

  using Base::operator=;

  template < size_t N > BasicStringRef(const CharT (&str)[N]) : BasicStringRef(str, N - 1) {}

  constexpr BasicStringRef(const CharT* str, size_t len) : Base(str, len) {}

  template < typename Allocator >
  BasicStringRef(const std::basic_string< CharT, Traits, Allocator >& str)
      : Base(str) {}

  /// Explicitly disallow construction from expiring string
  template < typename Allocator > BasicStringRef(std::basic_string< CharT, Traits, Allocator >&& str) = delete;

private:
  /// Hide all constructors
  using Base::basic_string_ref;
};

template < typename CharT, typename Traits > inline auto to_string(BasicStringRef< CharT, Traits > const& ref) {
  return ref.to_string();
}

using StringRef = BasicStringRef< char, ::std::char_traits< char > >;

} // inline namespace ext
} // namespace xi
