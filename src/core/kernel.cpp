#include "xi/core/kernel.h"

namespace xi {
namespace core {

  thread_local struct { volatile bool received = false; } volatile shutdown_signal;

  struct core_descriptor {
    hw::cpu const &cpu;
    unsigned id;
  };

  kernel::kernel() : kernel(hw::enumerate()) {
  }
  kernel::kernel(hw::machine m)
      : _machine(move(m))
      , _exception_filter([](auto) { return false; }) {
  }
  kernel::~kernel() {
    try {
      await_shutdown();
    } catch (...) {
    }
  }

  core::future<> kernel::start(unsigned count, unsigned per_core_queue_size) {
    if (count < 1) {
      return make_ready_future(
          make_exception_ptr(std::invalid_argument("Invalid core count.")));
    }
    if (_machine.cpus().size() < count) {
      return make_ready_future(make_exception_ptr(
          std::invalid_argument("Not enough cores available.")));
    }
    _shard_queue_size = per_core_queue_size;
    _shards.resize(count);

    return make_ready_future();
  }

  void kernel::startup(u16 id) {
    assert(nullptr == _shards[id]);
    this_shard  = make_shard(id);
    _shards[id] = this_shard;
    // _shards[id]->start();
  }

  void kernel::cleanup(u16 id) {
    assert(nullptr != _shards[id]);
    delete _shards[id];
    _shards[id] = nullptr;
  }

  mut<shard> kernel::make_shard(u16 id) {
    // return new shard(this);
    return nullptr;
  }

  void kernel::initiate_shutdown() {
    for (unsigned core = 0; core < core_count(); ++core) {
      post(core, [] { shutdown_signal.received = true; });
    }
  }

  void kernel::await_shutdown() {
    if (_exit_exception) {
      rethrow_exception(_exit_exception);
    }
  }

  void kernel::exception_filter(exception_filter_type filter) {
    _exception_filter = move(filter);
  }

  void kernel::handle_exception(exception_ptr ex) {
    if (!_exception_filter || !_exception_filter(ex)) {
      {
        auto lock       = make_unique_lock(_exception_lock);
        _exit_exception = move(ex);
      }
      initiate_shutdown();
    }
  }

  unsigned kernel::core_count() {
    return _shards.size();
  }

  size_t kernel::register_poller(u16 core, own< poller > poller) {
    if (core >= _shards.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    return _shards[core]->register_poller(move(poller));
  }

  void kernel::deregister_poller(u16 core, usize poller_id) {
    if (core >= _shards.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    return _shards[core]->deregister_poller(poller_id);
  }

  void kernel::run_on_core(unsigned id) {
    std::cout << "starting thread: " << pthread_self() << std::endl;
    startup(id);
    XI_SCOPE(exit) {
      cleanup(id);
      shutdown_signal.received = false;
    };
    while (!shutdown_signal.received) {
      poll_core(id);
    }
  }

  void kernel::poll_core(unsigned id) {
    if (id >= _shards.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    _shards[id]->poll();
  }
}
}
