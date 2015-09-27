#pragma once

#define BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE

#include "xi/ext/Atomic.h"
#include "xi/ext/Chrono.h"
#include "xi/ext/Common.h"
#include "xi/ext/Error.h"
#include "xi/ext/Exception.h"
#include "xi/ext/Expected.h"
#include "xi/ext/FastCast.h"
#include "xi/ext/Hash.h"
#include "xi/ext/Lock.h"
#include "xi/ext/Likely.h"
#include "xi/ext/Meta.h"
#include "xi/ext/Optional.h"
#include "xi/ext/Pointer.h"
#include "xi/ext/Range.h"
#include "xi/ext/Require.h"
#include "xi/ext/Scope.h"
#include "xi/ext/String.h"
#include "xi/ext/TypeTraits.h"
#include "xi/ext/Tuple.h"

/// Include files that require the declarations above
#include "xi/util/Ownership.h"
