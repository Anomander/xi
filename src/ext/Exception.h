#pragma once

#include <exception>

namespace xi {
inline namespace ext {

using ::std::exception;
using ::std::exception_ptr;
using ::std::make_exception_ptr;
using ::std::current_exception;
using ::std::rethrow_exception;

} // inline namespace ext
} // namespace xi
