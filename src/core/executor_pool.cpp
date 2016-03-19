#include "xi/ext/configure.h"
#include "xi/core/kernel.h"
#include "xi/core/executor_pool.h"

namespace xi {
namespace core {

  executor_pool::executor_pool(mut< kernel > k, vector< u16 > ids) {
    for (auto id : ids) {
      _shards.push_back(k->mut_shard(id));
    }
  }

  executor_pool::executor_pool(mut< kernel > k, u16 count) {
    std::cout << count << " cores" << std::endl;
    for (u16 id = 0; id < count; ++id) {
      std::cout << k->mut_shard(id) << std::endl;
      _shards.push_back(k->mut_shard(id));
    }
  }

  executor_pool::~executor_pool() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
  }

  size_t executor_pool::size() const {
    return _shards.size();
  }

  mut< shard > executor_pool::executor_for_core(unsigned id) {
    if (id > _shards.size()) {
      throw std::invalid_argument("Executor with id " + to_string(id) +
                                  " is not registered");
    }
    return _shards[id];
  }

  mut< shard > executor_pool::next_executor() {
    return _shards[(_next_core++) % _shards.size()];
  }
}
}
