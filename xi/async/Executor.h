#pragma once

#include "xi/ext/Configure.h"
#include "xi/async/Context.h"
#include "xi/async/TaskQueue.h"

namespace xi {
namespace async {

  class Executor {
  public:
    Executor(size_t id, TaskQueue **queues, size_t coreCount) : _id(id), _queues(queues), _cores(coreCount) {}
    Executor() {
      setLocal< Executor >(*this);
      assert(&local< Executor >() == this);
    }
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

}
}
