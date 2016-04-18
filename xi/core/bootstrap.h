#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/barrier.h"
#include "xi/core/configuration.h"
#include "xi/core/message_bus.h"

namespace xi {
namespace core {

  class thread_launcher {
    vector< thread > _threads;

  public:
    void launch(function< void() > f) {
      _threads.emplace_back(thread(move(f)));
    }

    void join_all() {
      for (auto& thread : _threads) {
        thread.join();
      }
    }
  };

  class bootstrap {
    static bootstrap* INSTANCE;

    u8 _cpu_count = 0;
    configuration _config;
    shared_ptr< barrier > _barrier;
    thread_launcher _launcher;
    shard** _shards;
    message_bus** _message_buses;
    deque< function< void() > > _before_shutdown;

  protected:
    bootstrap() {
      assert(!INSTANCE);
      INSTANCE = this;
    }

    template < class Backend >
    bootstrap& configure(int argc, char* argv[]) {
      try {
        _config.init< Backend >().read_command_line(argc, argv);
      } catch (ref< exception > e) {
        std::cerr << "configuration error: " << e.what() << std::endl;
        ::exit(2);
      }
      // TODO: configure core count
      _cpu_count     = (argc > 2 ? atoi(argv[2]) : 1);
      _shards        = new shard*[_cpu_count];
      _message_buses = new message_bus*[_cpu_count];
      _barrier       = make_shared< barrier >(_cpu_count);
      for (u8 i : range::between< u8 >(1, _cpu_count)) {
        _launcher.launch([i, this] {
          _prepare_shard(i);
          _run_shard();
        });
      }
      _prepare_shard(0);
      _create_queues(_cpu_count);
      return *this;
    }

    bootstrap& before_shutdown(function< void() >&& f) {
      _before_shutdown.push_front(move(f));
      return *this;
    }

  public:
    void run();

    static u8 cpus() {
      return INSTANCE->_cpu_count;
    }

    static mut< shard > shard_at(u16 id) {
      return INSTANCE->_shards[id];
    }

    static void initiate_shutdown() {
      INSTANCE->_initiate_shutdown();
    }

  private:
    void _prepare_shard(u8);
    void _run_shard();
    void _create_queues(u8);
    void _initiate_shutdown();
  };
}
}
