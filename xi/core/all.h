#pragma once

#include "xi/core/channel.h"
#include "xi/core/coordinator.h"
#include "xi/core/generic_resumable.h"
#include "xi/core/policy/all.h"
#include "xi/core/reactor/all.h"
#include "xi/core/runtime.h"
#include "xi/core/sleep.h"
#include "xi/core/spawn.h"
#include "xi/core/worker.h"
#include "xi/core/yield.h"

// Bringing some inner names into a quickly accessible space
namespace xi {
inline namespace prelude {
  using core::spawn;
  using core::sleep;
  using core::yield;
}
}
