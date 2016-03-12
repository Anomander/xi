// #include "xi/io/buf/iterator.h"

// namespace xi {
// namespace io {

//   bool buffer::chain::iterator::equal(iterator const& other) const {
//     assert(other._head == _head);
//     return _pos == other._pos;
//   }

//   void buffer::chain::iterator::increment() {
//     if (++_pos >= _curr->end()) {
//       if (XI_UNLIKELY(_curr->_next == _head)) { _pos = _curr->end(); } else {
//         _curr = _curr->_next;
//         _pos = _curr->begin();
//       }
//     }
//   }
//   void buffer::chain::iterator::decrement() {
//     if (--_pos < _curr->begin()) {
//       if (XI_LIKELY(_curr != _head)) { _curr = _curr->_prev; }
//       _pos = _curr->begin();
//     }
//   }
//   void buffer::chain::iterator::advance(int n) {
//     while (n < 0) {
//       n += distance(_curr->begin(), _pos);
//       _pos = (_curr = _curr->_prev)->end() - 1;
//     }
//     while (n > 0) {
//       auto len = distance(_pos, _curr->end());
//       if (n >= len) {
//         _pos = (_curr = _curr->_next)->begin();
//         n -= len;
//       } else {
//         _pos += n;
//         n = 0;
//       }
//     }
//   }
//   int buffer::chain::iterator::distance_to(iterator const& other) const {
//     assert(other._head == _head);
//     if (other._curr == _curr) { return other._pos - _pos; } else {
//       int result = _curr->end() - _pos + (other._pos - other._curr->end());
//       auto p = _curr->_next;
//       auto end = other._curr;
//       uint8_t sign = 1;
//       while (p != end) {
//         if (p == _head) {
//           result = other._curr->end() - other._pos + (_pos - _curr->end());
//           p = end->_next;
//           end = _curr;
//           sign = -1;
//         }
//         result += p->size();
//         p = p->_next;
//       }
//       return sign + result;
//     }
//   }
// }
// }
