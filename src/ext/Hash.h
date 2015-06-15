#pragma once

#include <functional>
#include <boost/functional/hash.hpp>

namespace xi { 
inline namespace ext {

    template<class T>
    std::size_t hash_value(T && t) { return t.hashCode(); }

} // inline namespace ext
} // namespace xi
