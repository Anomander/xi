#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/lockfree.h"
#include "xi/core/detail/intrusive.h"
#include "xi/core/reactor/epoll.h"
#include "xi/core/worker.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace core {

  extern thread_local struct worker_stats {
    u64 tasks_ready     = 0;
    u64 max_tasks_ready = 0;
  } WORKER_STATS;

  namespace policy {

    class steady_clock_authority {
    public:
      using clock_type      = steady_clock;
      using time_point_type = steady_clock::time_point;

      static time_point_type max() noexcept {
        return time_point_type::max();
      }

      static nanoseconds max_duration() noexcept {
        return (max() - now());
      }

      static nanoseconds duration_till(time_point_type tp) noexcept {
        return (tp - steady_clock::now());
      }

      static time_point_type now() noexcept {
        return steady_clock::now();
      }

      static time_point_type now_plus(nanoseconds ns) noexcept {
        return steady_clock::now() + ns;
      }
    };

    template < class Reactor, class TimeAuthority >
    class generic_worker_isolation {
      struct worker_data {
        detail::ready_queue_type<resumable> ready_queue;
        detail::sleep_queue_type<resumable> sleep_queue;
        lockfree::queue< resumable* > external_queue{256};
        Reactor reactor;
        TimeAuthority time;
        typename TimeAuthority::time_point_type next_wakeup =
            TimeAuthority::max();
      };

      using worker_t = worker< generic_worker_isolation >;

      struct coordinator_data {
        spin_lock lock;
        vector< unique_ptr< worker_t > > workers;
        atomic< u64 > next_worker{0};
      };

      using worker_access_t = worker_access< generic_worker_isolation >;
      using coordinator_access_t =
          coordinator_access< generic_worker_isolation >;

    public:
      using worker_data_type      = worker_data;
      using coordinator_data_type = coordinator_data;

    public:
      resumable* pop(worker_access_t* w) {
        auto& data = w->data();
        for (;;) {
          _drain_external_queue(w);
          _drain_sleep_queue(w);
          _poll_reactor(w);
          if (XI_LIKELY(!data.ready_queue.empty())) {
            auto r = &data.ready_queue.front();
            r->ready_unlink();
            --WORKER_STATS.tasks_ready;
            return r;
          }
        }
      }

      void begin_task(worker_access_t* w, resumable*) {
        // w->data().reactor.begin_task_quota_monitor(10ms);
      }

      void end_task(worker_access_t* w, resumable*) {
        // w->data().reactor.end_task_quota_monitor();
      }

      void push_externally(worker_access_t* w, resumable* r) {
        printf("Pushing external task for %p : %p.\n", w, r);
        w->data().external_queue.push(r);
        maybe_wakeup(w);
      }

      void push_internally(worker_access_t* w, resumable* r) {
        auto tr = ++WORKER_STATS.tasks_ready;
        if (tr > WORKER_STATS.max_tasks_ready) {
          WORKER_STATS.max_tasks_ready = tr;
        }
        // r->sleep_unlink();
        w->data().ready_queue.push_back(*r);
      }

      void sleep_for(worker_access_t* w, resumable* r, nanoseconds ns) {
        auto&& data = w->data();
        r->ready_unlink();
        auto wakeup_time = data.time.now_plus(ns);
        r->wakeup_time(wakeup_time);
        data.sleep_queue.insert(*r);
        if (data.next_wakeup > wakeup_time) {
          data.next_wakeup = wakeup_time;
        }
        assert(r->is_sleep_linked());
        assert(!r->is_ready_linked());
      }

      void maybe_wakeup(worker_access_t* w) {
        atomic_signal_fence(memory_order_seq_cst);
        w->data().reactor.maybe_wakeup();
      }

      abstract_reactor& reactor(worker_access_t* w) {
        return w->data().reactor;
      }

      worker_t* start_worker(coordinator_access_t* c) {
        printf("Start worker\n");
        auto w   = make_unique< worker_t >();
        auto ret = w.get();
        {
          auto lock = make_unique_lock(c->data().lock);
          c->data().workers.emplace_back(move(w));
        }
        return ret;
      }

      void stop_worker(coordinator_access_t* c, worker_t* w) {
        auto& workers = c->data().workers;
        erase(remove_if(begin(workers),
                        end(workers),
                        [w](auto& it) { return it == w; }),
              end(workers));
      }

      worker_t* current_worker(coordinator_access_t*) {
        return reinterpret_cast< worker_t* >(runtime.local_worker());
      }

      worker_t* next_worker(coordinator_access_t* c) {
        auto&& data = c->data();
        if (XI_UNLIKELY(data.workers.empty())) {
          printf("Workers empty\n");
          return nullptr;
        }
        // printf("Workers: %lu\n", data.workers.size());
        auto idx = data.next_worker.fetch_add(1, memory_order_relaxed) %
                   data.workers.size();
        // printf("Worker: %lu\n", idx);
        return data.workers[idx].get();
      }

      void central_schedule(coordinator_access_t* c, resumable* r) {
        auto w = next_worker(c);
        w->attach_resumable(r);
        push_externally(w, r);
        maybe_wakeup(w);
      }

    private:
      void _poll_reactor(worker_access_t* w) {
        auto& data     = w->data();
        auto sleep_for = data.ready_queue.empty()
                             ? data.time.duration_till(data.next_wakeup)
                             : (0ms);
        data.reactor.poll_for(sleep_for);
      }
      void _drain_external_queue(worker_access_t* w) {
        auto& data = w->data();
        data.external_queue.consume_all(
            [&](resumable* r) { push_internally(w, r); });
      }
      void _drain_sleep_queue(worker_access_t* w) {
        auto& data = w->data();
        auto now   = data.time.now() + 100us;
        if (now >= data.next_wakeup) {
          auto i   = data.sleep_queue.begin();
          auto end = data.sleep_queue.end();
          for (; i != end;) {
            if (XI_LIKELY(i->wakeup_time() <= now)) {
              auto r = &(*i);
              i      = data.sleep_queue.erase(i);
              push_internally(w, r);
            } else {
              data.next_wakeup = i->wakeup_time();
              break;
            }
          }
          if (i == end) {
            data.next_wakeup = data.time.max();
          }
        }
      }
    };

    using worker_isolation =
        generic_worker_isolation< reactor::epoll, steady_clock_authority >;
  }
}
}
