#include "xi/core/kernel.h"

namespace xi {
namespace core {

  struct shutdown_signal {
    volatile bool received = false;
  };

  struct core_descriptor {
    hw::cpu const &cpu;
    unsigned id;
  };

  kernel::kernel() : kernel(hw::enumerate()) {}
  kernel::kernel(hw::machine m)
      : _machine(move(m))
      , _exception_filter([](auto exception) { return false; }) {}
  kernel::~kernel() {
    try {
      await_shutdown();
    } catch (...) {}
  }

  void kernel::start(unsigned count, unsigned per_core_queue_size) {
    if (count < 1) { throw std::invalid_argument("Invalid core count."); }
    if (_machine.cpus().size() < count) {
      throw std::invalid_argument("Not enough cores available.");
    }
    _queues.resize(count);
    _pollers.resize(count);
    _inbound_task_queues.resize(count);

    for (unsigned dst = 0; dst < count; ++dst) {
      _queues[dst].resize(count);
      for (unsigned src = 0; src < count; ++src) {
        _queues[dst][src] = make< task_queue >(per_core_queue_size);
        std::cout << "Created queue " << src << "->" << dst << " @ "
                  << address_of(_queues[dst][src]) << std::endl;
      }
    }
  }

  void kernel::startup(unsigned id) {
    xi::async::set_local< core_descriptor >(
        *new core_descriptor{machine().core(id), id});
  }

  void kernel::cleanup() {
    delete xi::async::try_local< core_descriptor >();
    xi::async::reset_local< core_descriptor >();
  }

  void kernel::initiate_shutdown() {
    for (unsigned core = 0; core < core_count(); ++core) {
      post(core, [] { xi::async::local< shutdown_signal >().received = true; });
    }
  }

  void kernel::await_shutdown() {
    if (_exit_exception) { rethrow_exception(_exit_exception); }
  }
  void kernel::exception_filter(exception_filter_type filter) {
    _exception_filter = move(filter);
  }
  void kernel::handle_exception(exception_ptr ex) {
    if (!_exception_filter || !_exception_filter(ex)) {
      {
        auto lock = make_unique_lock(_exception_lock);
        _exit_exception = move(ex);
      }
      initiate_shutdown();
    }
  }

  unsigned kernel::core_count() { return _queues.size(); }

  size_t kernel::register_poller(unsigned core, own< poller > poller) {
    if (core > _pollers.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    auto ret = reinterpret_cast< size_t >(address_of(poller));
    dispatch(core, [ poller = move(poller), this, core ]() mutable {
      _pollers[core].emplace_back(move(poller));
    });
    return ret;
  }

  void kernel::deregister_poller(unsigned core, size_t poller_id) {
    if (core > _pollers.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    dispatch(core, [poller_id, this, core] {
      auto &pollers = _pollers[core];
      pollers.erase(
          remove_if(begin(pollers), end(pollers), [&](auto const &poller) {
            return reinterpret_cast< size_t >(address_of(poller)) == poller_id;
          }),
          end(pollers));
    });
  }

  opt< unsigned > kernel::local_core_id() {
    auto *desc = xi::async::try_local< core_descriptor >();
    if (desc) { return desc->id; }
    return none;
  }

  void kernel::run_on_core(unsigned id) {
    std::cout << "starting thread: " << pthread_self() << std::endl;
    startup(id);
    XI_SCOPE(exit) { cleanup(); };
    shutdown_signal shutdown;
    xi::async::set_local< shutdown_signal >(shutdown);
    XI_SCOPE(exit) { xi::async::reset_local< shutdown_signal >(); };
    while (!shutdown.received) { poll_core(id); }
  }

  void kernel::poll_core(unsigned id) {
    try {
      for (auto &&p : _pollers[id]) { p->poll(); }
      for (auto &&q : _queues[id]) { q->process_tasks(); }

      if (XI_UNLIKELY(!_inbound_task_queues[id].tasks.empty())) {
        auto lock = make_lock(_inbound_task_queues[id].lock);
        auto &tasks = _inbound_task_queues[id].tasks;
        while (!tasks.empty()) {
          auto &next_task = tasks.back();
          XI_SCOPE(exit) {
            tasks.pop();
          }; // pop no matter what, we don't want to retry throwing tasks
          next_task->run();
        }
      }
    } catch (...) { handle_exception(current_exception()); }
  }
}
}
