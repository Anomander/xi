#include "xi/core/kernel.h"

namespace xi {
namespace test {

  enum : unsigned { kCurrentThread = 0, kOtherThread = 1 };

  class mock_kernel : public virtual ownership::unique {
    bool _shutdown_initiated = false;
    core::shard* _shards[2];

  public:
    mock_kernel() {
      _shards[0]       = new core::shard(kCurrentThread, nullptr);
      _shards[1]       = new core::shard(kOtherThread, nullptr);
      for (auto && s : _shards) {
        core::this_shard = s;
        s->init();
      }
      core::this_shard = _shards[0];
    }

    ~mock_kernel() {
      for (auto && s : _shards) {
        s->shutdown();
      }
      for (auto& s : _shards) {
        delete s;
      }
    }

    mut<core::shard> shard_for_core(unsigned id) {
      return _shards[id];
    }

  public:
    void run_core(unsigned id) {
      core::this_shard = _shards[ id ];
      _shards[id]->poll();
    }
  };
}
}
