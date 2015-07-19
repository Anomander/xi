#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace xi {
inline namespace ext {

using ::boost::uuids::uuid;

template < class > struct TaggedType {
  TaggedType() noexcept = default;
  static auto const& getTypeUUID() noexcept {
    static const uuid TAG{ ::boost::uuids::random_generator() };
    return TAG;
  }
};

} // inline namespace ext
} // namespace xi
