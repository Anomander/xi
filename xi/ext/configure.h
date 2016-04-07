#pragma once

#define BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE

#include "xi/ext/align.h"
#include "xi/ext/atomic.h"
#include "xi/ext/chrono.h"
#include "xi/ext/class_semantics.h"
#include "xi/ext/common.h"
#include "xi/ext/container.h"
#include "xi/ext/error.h"
#include "xi/ext/exception.h"
#include "xi/ext/expected.h"
#include "xi/ext/fast_cast.h"
#include "xi/ext/intrusive.h"
#include "xi/ext/likely.h"
#include "xi/ext/lock.h"
#include "xi/ext/meta.h"
#include "xi/ext/number.h"
#include "xi/ext/pointer.h"
#include "xi/ext/range.h"
#include "xi/ext/require.h"
#include "xi/ext/scope.h"
#include "xi/ext/string.h"
#include "xi/ext/tuple.h"
#include "xi/ext/type_traits.h"
#include "xi/ext/types.h"

/// include files that require the declarations above
#include "xi/util/optional.h"
#include "xi/util/ownership.h"
