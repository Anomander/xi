#pragma once

#include "xi/ext/configure.h"

#ifdef XI_HAS_HWLOC
#include <hwloc.h>

namespace xi {
namespace hw {

  class cpu {
    struct {
      int os;
      int numa;
    } _id;

  public:
    cpu(hwloc_obj_t cpu);

    auto id() const noexcept { return _id; }
  };

  class machine {
    vector< cpu > _cpus;

  public:
    machine(hwloc_topology_t topology);

    decltype(auto) cpus() const { return _cpus; }
    cpu const &core(unsigned c) const { return _cpus.at(c); }
  };

  extern machine enumerate();
}
}

#else  // XI_HAS_HWLOC

namespace xi {
namespace hw {

  class cpu {
    struct {
      unsigned os;
      unsigned numa;
    } _id;

  public:
    cpu(unsigned);

    auto id() const noexcept { return _id; }
  };

  class machine {
    vector< cpu > _cpus;

  public:
    machine();

    decltype(auto) cpus() const { return _cpus; }
    cpu const &core(unsigned core) const { return _cpus.at(core); }
  };

  extern machine enumerate();
}
}
#endif // XI_HAS_HWLOC
