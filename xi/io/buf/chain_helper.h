// #pragma once

// #include "xi/io/buf/chain.h"
// #include "xi/io/buf/iterator.h"

// namespace xi {
// namespace io {

//   struct buffer::chain::helper {
//     static void insert_before(mut< buffer > a, mut< buffer > b) {
//       insert_after(a->_prev, b);
//     }
//     static void insert_after(mut< buffer > a, mut< buffer > b) {
//       insert_chain_after(a, b, b->_prev);
//     }
//     static void insert_chain_after(mut< buffer > a, mut< buffer > begin,
//                                    mut< buffer > end) {
//       a->_next->_prev = end;
//       end->_next = a->_next;
//       a->_next = begin;
//       begin->_prev = a;
//     }

//     static size_t subchain_length(mut< buffer > beg, mut< buffer > end) {
//       auto p = beg->_next;
//       auto result = beg->size();
//       while (p != end) {
//         result += p->size();
//         p = p->_next;
//       }
//       return result;
//     }

//     static void reset_buffer(mut<buffer> b) {
//       b->_begin = b->_arena->data() + b->_offset;
//       b->_end = b->_begin;
//     }
//   };

// }
// }
