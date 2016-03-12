// #pragma once

// #include "xi/io/buf/buffer.h"

// namespace xi {
// namespace io {

//   class buffer::view final {
//     own< arena_base > _arena;
//     size_t _offset, _size;

//   private:
//     friend class buffer;

//     view(own< arena_base > arena, size_t offset, size_t length)
//         : _arena(move(arena)), _offset(offset), _size(length) {}

//   public:
//     uint8_t const* begin() const;
//     uint8_t const* end() const;
//     size_t size() const;
//   };

//   inline uint8_t const* buffer::view::begin() const {
//     return _arena->data() + _offset;
//   }
//   inline uint8_t const* buffer::view::end() const { return begin() + _size; }
//   inline size_t buffer::view::size() const { return _size; }
// }
// }
