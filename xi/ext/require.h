#pragma once

#include "xi/ext/meta.h"
#include "xi/ext/type_traits.h"

#define XI_REQUIRE_DECL(...)                                                   \
  ::xi::meta::enable_if< __VA_ARGS__ > = ::xi::meta::enabled::value

#define XI_OVERLOAD_IF(Condition, Return)                                      \
  ::xi::meta::enable_if< Condition, Return >

#define XI_OVERLOAD_UNLESS(Condition, Return)                                  \
  ::xi::meta::disable_if< Condition, Return >

#define XI_UNLESS_DECL(...)                                                    \
  ::xi::meta::disable_if< __VA_ARGS__ > = ::xi::meta::enabled::value

#define XI_REQUIRE(...) ::xi::meta::enable_if< __VA_ARGS__ >

#define XI_UNLESS(...) ::xi::meta::disable_if< __VA_ARGS__ >

#define XI_SPECIALIZABLE class __TRequirement = void

#define XI_SELECT_SPECIALIZATION ::xi::meta::detail::enabler

#define XI_INVALID_SPECIALIZATION(...)                                         \
  static_assert(std::is_same< __TRequirement, void >::value, __VA_ARGS__)

#define XI_SPECIALIZE_IF(...) XI_REQUIRE(__VA_ARGS__)

#define XI_SPECIALIZE_UNLESS(...) ::xi::meta::disable_if< __VA_ARGS__ >

#ifdef XI_HAS_DOLLAR_MACROS
#define require$(...) XI_REQUIRE(__VA_ARGS__)
#define required$(...) XI_REQUIRE_DECL(__VA_ARGS__)
#define unless$(...) XI_UNLESS(__VA_ARGS__)
#endif // XI_HAS_DOLLAR_MACROS
