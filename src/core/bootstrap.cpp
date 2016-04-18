#include "xi/ext/configure.h"
#include "xi/core/memory/memory.h"
#include "xi/core/shard.h"
#include "xi/core/signals.h"
#include "xi/server_bootstrap.h"

namespace xi {
namespace core {

  namespace {
    void pin(u8 cpu) {
      cpu_set_t cs;
      CPU_ZERO(&cs);
      // CPU_SET(_cpu.id().os, &cs);
      CPU_SET(cpu, &cs);
      auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
      assert(r == 0);
    }
  }

  bootstrap* bootstrap::INSTANCE = nullptr;

  void bootstrap::run() {
    _shards[0]->_signals->handle(SIGINT, [] { initiate_shutdown(); });
    _run_shard();
    _launcher.join_all();
  }

  void bootstrap::_initiate_shutdown() {
    for (auto&& f : _before_shutdown) {
      _shards[0]->dispatch([f = move(f)] { f(); });
    }
    _before_shutdown.clear();
    for (auto i : range::to(cpus())) {
      _shards[i]->shutdown();
    }
  }

  void bootstrap::_prepare_shard(u8 cpu) {
    // TODO: Configure
    pin(cpu);
    memory::resource r;
    r.bytes  = 4ull << 30;
    r.nodeid = 0;
    memory::configure({r}, none);

    _shards[cpu] = new core::shard{cpu, _message_buses};
    this_shard   = _shards[cpu];
    _shards[cpu]->init();
  }

  void bootstrap::_run_shard() {
    _barrier->wait();
    XI_SCOPE(exit) {
      delete this_shard;
    };
    this_shard->run();
  }

  void bootstrap::_create_queues(u8 cores) {
    for (auto i : range::to(cores)) {
      _message_buses[i] = (message_bus*)malloc(sizeof(message_bus) * cores);
      for (auto j : range::to(cores)) {
        new (&_message_buses[i][j]) message_bus(_shards[i], _shards[j]);
      }
    }
  }
}
}
