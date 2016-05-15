#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  namespace policy {
    template < class Policy >
    struct generic_access {
      Policy policy;
    };

    template < class Policy >
    class worker_access : public generic_access<Policy> {
      using data_t = typename Policy::worker_data_type;

      data_t _data;

    private:
      friend Policy;
      data_t& data() {
        return _data;
      }
    };

    template < class Policy >
    class coordinator_access : public generic_access<Policy> {
      using data_t = typename Policy::coordinator_data_type;

      data_t _data;

    private:
      friend Policy;
      data_t& data() {
        return _data;
      }
    };
  }
}
}
