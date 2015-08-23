#pragma once

#include "ext/Configure.h"

#include <hwloc.h>

namespace xi {
namespace hw {

  class Cpu {
    struct {
      int os;
      int numa;
    } _id;

  public:
    Cpu(hwloc_obj_t cpu);

    auto id() const noexcept { return _id; }
  };

  class Machine {
    vector< Cpu > _cpus;

  public:
    Machine(hwloc_topology_t topology);

    decltype(auto) cpus() const { return _cpus; }
    Cpu const & cpu(unsigned core) const { return _cpus.at(core); }
  };

  extern Machine enumerate();
}
}
