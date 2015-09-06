#include "core/Kernel.h"

namespace xi {
namespace core {

  struct ShutdownSignal {
    volatile bool received = false;
  };

  Kernel::Kernel() : _machine(hw::enumerate()) {}
  Kernel::Kernel(hw::Machine m) : _machine(move(m)) {}
  Kernel::~Kernel() {
    try {
      awaitShutdown();
    } catch (...) {
    }
  }

  void Kernel::start(unsigned count, unsigned perCoreQueueSize) {
    if (_machine.cpus().size() < count) {
      throw std::invalid_argument("Not enough cores available.");
    }
    _queues.resize(count);
    _pollers.resize(count);
    _inboundTaskQueues.resize(count);

    for (unsigned dst = 0; dst < count; ++dst) {
      _queues[dst].resize(count);
      for (unsigned src = 0; src < count; ++src) {
        _queues[dst][src] = make< TaskQueue >(perCoreQueueSize);
        std::cout << "Created queue " << src << "->" << dst << " @ " << addressOf(_queues[dst][src]) << std::endl;
      }
    }
  }

  void Kernel::startup(unsigned id) { async::setLocal< CoreDescriptor >(*new CoreDescriptor{machine().cpu(id), id}); }

  void Kernel::cleanup() {
    delete async::tryLocal< CoreDescriptor >();
    async::resetLocal< CoreDescriptor >();
  }

  void Kernel::initiateShutdown() {
    for (unsigned core = 0; core < coreCount(); ++core) {
      post(core, [] { async::local< ShutdownSignal >().received = true; });
    }
  }

  unsigned Kernel::coreCount() { return _queues.size(); }

  size_t Kernel::registerPoller(unsigned core, own< Poller > poller) {
    if (core > _pollers.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    auto ret = reinterpret_cast< size_t >(addressOf(poller));
    dispatch(core, [ poller = move(poller), this, core ] () mutable { _pollers[core].emplace_back(move(poller)); });
    return ret;
  }

  void Kernel::deregisterPoller(unsigned core, size_t pollerId) {
    if (core > _pollers.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    dispatch(core, [pollerId, this, core] {
      auto &pollers = _pollers[core];
      pollers.erase(remove_if(begin(pollers), end(pollers),
                              [&](auto const & poller) { return reinterpret_cast< size_t >(addressOf(poller)) == pollerId; }),
                    end(pollers));
    });
  }

  unsigned Kernel::localCoreId() {
    auto *desc = async::tryLocal< CoreDescriptor >();
    if (desc) {
      return desc->id;
    }
    return -1;
  }

  void Kernel::runOnCore(unsigned id) {
    std::cout << "starting thread: " << pthread_self() << std::endl;
    startup(id);
    XI_SCOPE(exit) { cleanup(); };
    ShutdownSignal shutdown;
    async::setLocal< ShutdownSignal >(shutdown);
    while (!shutdown.received) {
      pollCore(id);
    }
  }

  void Kernel::pollCore(unsigned id) {
    for (auto &&p : _pollers[id]) {
      p->poll();
    }
    for (auto &&q : _queues[id]) {
      q->processTasks();
    }

    if (!_inboundTaskQueues[id].tasks.empty()) {
      auto lock = makeLock(_inboundTaskQueues[id].lock);
      auto &tasks = _inboundTaskQueues[id].tasks;
      while (!tasks.empty()) {
        auto &nextTask = tasks.back();
        XI_SCOPE(exit) { tasks.pop(); }; // pop no matter what, we don't want to retry throwing tasks
        try {
          std::cout << "Running from inbound queue: " << addressOf(nextTask) << std::endl;
          nextTask->run();
        } catch (...) {
          // ignore task exception
        }
      }
    }
  }
}
}
