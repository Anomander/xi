// #pragma once

// #include "xi/ext/configure.h"
// #include "xi/core/task.h"

// namespace xi {
// namespace core {

//   class shard;

//   class executor_pool : public virtual ownership::std_shared {
//     vector< mut< shard > > _shards;
//     unsigned _next_core = 0;

//   public:
//     executor_pool(initializer_list< u16 > ids);

//     executor_pool(u16 count);

//     ~executor_pool();

//     size_t size() const;

//     template < class F >
//     void post_on_all(F const & f) {
//       for (auto &e : _shards) {
//         e->post(f); // copy to all shards
//       }
//     }

//     template < class F >
//     void post_on(size_t core, F &&f) {
//       executor_for_core(core)->post(forward< F >(f));
//     }

//     template < class F >
//     void post(F &&f) {
//       next_executor()->post(forward< F >(f));
//     }

//     template < class F >
//     void dispatch(F &&f) {
//       next_executor()->dispatch(forward< F >(f));
//     }

//     mut< shard > executor_for_core(unsigned id);

//     mut< shard > next_executor();

//     void post_on(u16, own<task>);
//   };
// }
// }
