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
      , _exception_filter([](auto exception) { return false; }) {
  }
  kernel::~kernel() {
    try {
      await_shutdown();
    } catch (...) {
    }
  }

  async::future<> kernel::start(unsigned count, unsigned per_core_queue_size) {
    if (count < 1) {
      return async::make_ready_future(
          make_exception_ptr(std::invalid_argument("Invalid core count.")));
    }
    if (_machine.cpus().size() < count) {
      return async::make_ready_future(make_exception_ptr(
          std::invalid_argument("Not enough cores available.")));
    }
    _queues.resize(count);
    _shards.resize(count);

    for (unsigned dst = 0; dst < count; ++dst) {
      _queues[dst].resize(count);
      for (unsigned src = 0; src < count; ++src) {
        _queues[dst][src] = make< task_queue >(per_core_queue_size);
        std::cout << "Created queue " << src << "->" << dst << " @ "
                  << address_of(_queues[dst][src]) << std::endl;
      }
    }

    return async::make_ready_future();
  }

  void kernel::startup(u16 id) {
    assert(nullptr == _shards[id]);
    this_shard  = make_shard(id);
    _shards[id] = this_shard;
  }

  void kernel::cleanup(u16 id) {
    assert(nullptr != _shards[id]);
    delete _shards[id];
    _shards[id] = nullptr;
  }

  mut<shard> kernel::make_shard(u16 id) {
    return new shard(this, id, _queues);
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
    return _queues.size();
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
    std::chrono::nanoseconds time = 0ns;
    usize iter = 0;
    while (!shutdown_signal.received) {
      auto start = std::chrono::high_resolution_clock::now();
      poll_core(id);
      time += std::chrono::high_resolution_clock::now() - start;
      ++iter;
    }
    std::cout << "running time: " << time.count() << std::endl;
    std::cout << "average time: " << time.count()/iter << std::endl;
  }

  void kernel::poll_core(unsigned id) {
    if (id >= _shards.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    _shards[id]->poll();
  }
}
}
