// #pragma once

// #include "xi/io/buf/chain.h"
// #include "xi/io/buf/iterator.h"

// namespace xi {
// namespace io {

//   class buffer::chain::cursor final {
//     mut< chain > _chain;
//     iterator _it;

//   public:
//     cursor(mut< chain >, iterator);

//     void read(void*, size_t);

//     template < class T, XI_REQUIRE_DECL(is_arithmetic< T >)> T read();

//     iterator position() const;

//     void skip(size_t);

//     bool is_at_end() const { return _it == _chain->end(); }
//   };

//   inline buffer::chain::cursor::cursor(mut< chain > c, iterator it)
//       : _chain(c), _it(it) {}

//   template < class T, XI_REQUIRE(is_arithmetic< T >)>
//   T buffer::chain::cursor::read() {
//     T t;
//     read((void*)address_of(t), sizeof(T));
//     return t;
//   }

//   inline auto buffer::chain::cursor::position() const -> iterator {
//     return _it;
//   }

//   inline void buffer::chain::cursor::skip(size_t n) { _it += n; }
// }
// }
