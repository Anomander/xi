#pragma once

#define BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE

#include "ext/Atomic.h"
#include "ext/Chrono.h"
#include "ext/Common.h"
#include "ext/Error.h"
#include "ext/Exception.h"
#include "ext/Expected.h"
#include "ext/FastCast.h"
#include "ext/Hash.h"
#include "ext/Lock.h"
#include "ext/Meta.h"
#include "ext/Optional.h"
#include "ext/Pointer.h"
#include "ext/Require.h"
#include "ext/Scope.h"
#include "ext/String.h"
#include "ext/TypeTraits.h"
#include "ext/Tuple.h"

/// Include files that require the declarations above
#include "util/Ownership.h"
#include "util/DefaultInitializable.h"
