#pragma once

#include <boost/variant.hpp>

namespace xi {
inline namespace ext {

  using ::boost::variant;
  using ::boost::get;
  using ::boost::static_visitor;

} // inline namespace ext
} // namespace xi
