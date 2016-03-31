#include "xi/io/fragment.h"

namespace xi {
namespace io {
  namespace {

    inline void fragment_deleter(fragment *f) {
      delete f;
    };

    template < class I >
    auto skip_offset(I start, I end, mut< usize > offset) {
      for (; start != end && *offset >= start->size(); ++start) {
        *offset -= start->size();
      }
      return start;
    }
  }
}
}
