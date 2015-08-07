#pragma once

#include "async/Context.h"

namespace xi {
namespace async2 {
  using namespace xi::async;

  class Executor {
  public:
    Executor(size_t id, TaskQueue **queues, size_t coreCount) : _id(id), _queues(queues), _cores(coreCount) {}
    Executor() = default;
    template < class Func >
    void submit(Func &&f) {
      auto &localExec = local< Executor >();
      auto &q = localExec.getOutboundQueue(id());
      q.submit(forward< Func >(f));
    }

    void processQueues() {
      for (size_t src = 0; src < _cores; ++src) {
        // if ( src == id () ) {
        //   continue;
        // }
        getInboundQueue(src).processTasks();
      }
    }

    virtual void run(function< void() > f) {}
    virtual void join() {}

    size_t id() const noexcept { return _id; }

  protected:
    virtual ~Executor() { cleanup(); }
    void setup() {
      setLocal< Executor >(*this);
      assert(&local< Executor >() == this);
    }

    void cleanup() { resetLocal< Executor >(); }

    TaskQueue &getOutboundQueue(size_t coreId) {
      // std::cout << "Queue for submission " << id() << "->" << coreId << " @ " << &_queues[coreId][id()] << std::endl;
      return _queues[coreId][id()];
    }

    TaskQueue &getInboundQueue(size_t coreId) {
      // std::cout << "Queue for submission " << id() << "->" << coreId << " @ "<<
      // &_queues[id()][coreId] << std::endl;
      return _queues[id()][coreId];
    }

  private:
    size_t _id;
    TaskQueue **_queues;
    size_t _cores;
  };

  template < class Func >
  void schedule(Func &&f) {
    local< Executor >().submit(forward< Func >(f));
  }

  class ThreadExecutor : public Executor {
  public:
    using Executor::Executor;
    void run(function< void() > f) noexcept override {
      try {
        _thread = boost::thread([ this, f = move(f) ] {
          try {
            this->setup();

            XI_SCOPE(exit) { this->cleanup(); };
            f();
          } catch (...) {
            std::cout << "Exception in executor " << id() << std::endl;
            // satisfy noexcept
          }
        });
      } catch (...) {
        // satisfy noexcept
      }
    }

    void join() override { _thread.join(); }

  public:
    boost::thread _thread;
  };

  template < class E >
  class ExecutorGroup : public virtual ownership::StdShared {
  public:
    ExecutorGroup(size_t count, size_t perCoreQueueSize) {
      _queues = new TaskQueue *[count];
      for (size_t dst = 0; dst < count; ++dst) {
        _queues[dst] = (TaskQueue *)malloc(sizeof(TaskQueue) * count);
        for (size_t src = 0; src < count; ++src) {
          new (&_queues[dst][src]) TaskQueue{perCoreQueueSize};
          std::cout << "Created queue " << src << "->" << dst << " @ " << &_queues[dst][src] << std::endl;
        }
      }
      for (size_t i = 0; i < count; ++i) {
        _executors.emplace_back(E{i, _queues, count});
      }
    }

    virtual ~ExecutorGroup() {
      for (size_t dst = 0; dst < _executors.size(); ++dst) {
        _queues[dst]->~TaskQueue();
        free(_queues[dst]);
      }
      _executors.clear();
      delete[] _queues;
    }

    template < class Func >
    void run(Func f) {
      for (auto &e : _executors) {
        e.run(f);
      }
      for (auto &e : _executors) {
        e.join();
      }
    }

    template < class Func >
    void executeOnAll(Func f) {
      for (auto &e : _executors) {
        e.submit(f);
      }
    }

    template < class Func >
    void executeOn(size_t core, Func &&f) {
      if (core > _executors.size()) {
        throw std::invalid_argument("Core not registered");
      }
      _executors[core].submit(forward< Func >(f));
    }

    template < class Func >
    void executeOnNext(Func &&f) {
      _executors[(_nextCore++) % _executors.size()].submit(forward< Func >(f));
    }

  private:
    vector< E > _executors;
    TaskQueue **_queues;
    size_t _nextCore = 0;
  };

  template < class E, class R >
  class Engine {
  public:
    void run(function< void() > f) {
      auto coreCount = 8UL;                 // get from config
      auto perCoreQueueSize = 100 * 1024UL; // get from config
      atomic< size_t > readyCount{0};
      _executors = make< ExecutorGroup< E > >(coreCount, perCoreQueueSize);
      _executors->run([&, f = move(f) ] {
        auto &exec = local< Executor >();
        auto &reactor = local< R >();

        std::cout << "Executor " << exec.id() << std::endl;
        _executors->executeOn((exec.id() + 1) % coreCount,
                              [&] { std::cout << "Hello from exec " << exec.id() << std::endl; });
        if (++readyCount == coreCount) {
          f();
        }
        for (;;) {
          reactor.poll();
          exec.processQueues();
        }
      });
    }

    template < class Func >
    void runOn(size_t core, Func &&f) {
      _executors->executeOn(core, forward< Func >(f));
    }

    template < class Func >
    void runOnNext(Func &&f) {
      _executors->executeOnNext(forward< Func >(f));
    }

  private:
    Engine() = default;
    template < class >
    friend class LocalStorage;

  private:
    own< ExecutorGroup< E > > _executors;
  };

  /// Override to always return the same Engine object.
  template < class E, class R >
  struct LocalStorage< Engine< E, R > > {
    static Engine< E, R > &get() {
      static Engine< E, R > OBJ;
      return OBJ;
    }
  };
}
}
