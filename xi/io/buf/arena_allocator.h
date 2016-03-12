// #include "xi/ext/configure.h"
// #include "xi/io/buf/buffer_allocator.h"
// #include "xi/io/buf/heap_buf.h"

// namespace xi {
// namespace io {

//   class arena_allocator : public buffer_allocator {
//     own< heap_buf > _current_arena;
//     size_t _page_size;

//   public:
//     arena_allocator(size_t page_size) : _page_size(page_size) {}

//   public:
//     own< buffer > allocate(size_t size) override {
//       validate(size);
//       return _current_arena->allocate_buffer(size)
//           .unwrap_or([&, this]() mutable {
//             _current_arena = make< heap_buf >(_page_size);
//             return _current_arena->allocate_buffer(size).unwrap();
//           });
//     }
//     own< buffer > reallocate(own< buffer > b, size_t size) override {
//       if (size <= b->size()) return move(b);
//       validate(size);
//       auto range = allocate(size);
//       copy(b->begin(), b->end(), range->begin());
//       return move(range);
//     }

//   private:
//     void validate(size_t size) {
//       if (size + detail::aligned_size(sizeof(buffer)) > _page_size) { throw std::bad_alloc(); }
//       if (!is_valid(_current_arena)) {
//         _current_arena = make< heap_buf >(_page_size);
//       }
//     }
//   };
// }
// }
