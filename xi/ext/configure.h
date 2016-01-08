#pragma once

#define BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE

#include "xi/ext/atomic.h"
#include "xi/ext/chrono.h"
#include "xi/ext/common.h"
#include "xi/ext/error.h"
#include "xi/ext/exception.h"
#include "xi/ext/expected.h"
#include "xi/ext/fast_cast.h"
#include "xi/ext/hash.h"
#include "xi/ext/lock.h"
#include "xi/ext/likely.h"
#include "xi/ext/meta.h"
#include "xi/ext/pointer.h"
#include "xi/ext/range.h"
#include "xi/ext/require.h"
#include "xi/ext/scope.h"
#include "xi/ext/string.h"
#include "xi/ext/types.h"
#include "xi/ext/type_traits.h"
#include "xi/ext/tuple.h"

/// include files that require the declarations above
#include "xi/util/ownership.h"
#include "xi/util/option.h"