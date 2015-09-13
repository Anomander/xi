#pragma once

#include "xi/ext/Configure.h"

#ifdef XI_HAS_HWLOC
#include <hwloc.h>

namespace xi {
namespace hw {

  class Cpu {
    struct {
      unsigned os;
      unsigned numa;
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

#else//XI_HAS_HWLOC

namespace xi {
  namespace hw {

    class Cpu {
      struct {
        unsigned os;
        unsigned numa;
      } _id;

    public:
      Cpu(unsigned cpu);

      auto id() const noexcept { return _id; }
    };

    class Machine {
      vector< Cpu > _cpus;

    public:
      Machine();

      decltype(auto) cpus() const { return _cpus; }
      Cpu const & cpu(unsigned core) const { return _cpus.at(core); }
    };

    extern Machine enumerate();
  }
}
#endif//XI_HAS_HWLOC
