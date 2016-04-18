#pragma once

#include "xi/ext/configure.h"
#include "xi/core/task.h"

namespace xi {
namespace core {

  class task_queue {
    deque< own< task > > _deque;

  public:
    task_queue() = default;

    template < class W >
    void submit(W &&work) {
      _submit(forward< W >(work), is_base_of< task, decay_t< W > >{});
    }

    void submit(own<task> t) {
      _deque.emplace_back(move(t));
    }

    void process_tasks() {
      auto cnt = 0;
      while (!_deque.empty()) {
        XI_SCOPE(exit) {
          _deque.pop_front();
        };
        _deque.front()->run();
        ++cnt;
      }
      // std::cout << "processed: " << cnt << std::endl;
    }

  private:
    template < class W >
    void _submit(W &&work, meta::false_type) {
      _push_task(make_task(forward< W >(work)));
    }
    template < class W >
    void _submit(W &&work, meta::true_type) {
      _push_task(forward< W >(work));
    }
    template < class T >
    void _push_task(T &&t) {
      _deque.emplace_back(make_unique< decay_t< T > >(forward< T >(t)));
      // std::cout << "queue size: " << _deque.size() << std::endl;
    }
  };
}
}
