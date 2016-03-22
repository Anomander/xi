#include "xi/io/fragment.h"

namespace xi {
namespace io {
  namespace {

    void fragment_deleter(fragment *f) {
      delete f;
    };

    template < class I >
    static auto find_fragment_by_size(I beg, I end, mut< usize > sz) {
      for (; beg != end && *sz > beg->size(); ++beg) {
        *sz -= beg->size();
      }
      return beg;
    }
  }
}
}
