#pragma once

#undef XI_LIKELY
#undef XI_UNLIKELY

#define XI_LIKELY(x) (__builtin_expect((x), 1))
#define XI_UNLIKELY(x) (__builtin_expect((x), 0))
