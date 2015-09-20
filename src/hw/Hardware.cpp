#include "xi/ext/Configure.h"
#include "xi/hw/Hardware.h"

namespace xi {
namespace hw {

#ifdef XI_HAS_HWLOC

  Cpu::Cpu(hwloc_obj_t cpu) : _id{hwloc_bitmap_first(cpu->cpuset), hwloc_bitmap_first(cpu->nodeset)} {}

  Machine::Machine(hwloc_topology_t topology) {
    auto depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CORE);
    auto ncores = hwloc_get_nbobjs_by_depth(topology, depth);
    for (unsigned core = 0; core < ncores; ++core) {
      auto cpu = hwloc_get_obj_by_depth(topology, depth, core);
      if (cpu) {
        _cpus.emplace_back(cpu);
      }
    }
  }

  Machine enumerate() {
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    XI_SCOPE(exit) { hwloc_topology_destroy(topology); };

    hwloc_topology_load(topology);

    return Machine{topology};
  }

#else // XI_HAS_HWLOC

  Cpu::Cpu(unsigned core) : _id{core, core} {}

  Machine::Machine() {
    for (unsigned core = 0; core < std::thread::hardware_concurrency(); ++core) {
      _cpus.emplace_back(core);
    }
  }

  Machine enumerate() { return Machine{}; }

#endif // XI_HAS_HWLOC
}
}
