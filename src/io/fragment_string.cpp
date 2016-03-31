#include "xi/io/fragment_string.h"
#include "xi/io/detail/buffer_utils.h"

namespace xi {
namespace io {

  fragment_string::fragment_string(list_t&& fragments, usize sz)
      : _fragments(move(fragments)), _size(sz) {
  }

  fragment_string::fragment_string(own< fragment > b) {
    if (!b->size() && !b->headroom() && !b->tailroom()) {
      return;
    }
    assert(b);
    _size = b->size();
    _fragments.push_back(*b.release());
  }

  fragment_string::~fragment_string() {
    _size = 0;
    _fragments.clear_and_dispose(fragment_deleter);
  }

  i32 fragment_string::compare(string_ref s) const {
    usize offset = 0;
    if (!s.size()) {
      return make_signed_t< usize >(_size);
    }
    for (auto&& frag : _fragments) {
      if (frag.size() == 0) {
        continue;
      }
      auto cap = min(frag.size(), s.size() - offset);
      auto ret = memcmp(frag.data(), s.data() + offset, cap);
      if (ret) {
        return ret;
      }
      offset += cap;
      if (offset == s.size()) {
        return make_signed_t< usize >(_size - offset);
      }
    }
    return make_signed_t< usize >(_size - s.size());
  }

  i32 fragment_string::compare(ref<fragment_string> s) const {
    if (!s.size()) {
      return make_signed_t< usize >(_size);
    }
    auto it_my = begin(_fragments);
    auto end_my = end(_fragments);
    auto it_other = begin(s._fragments);
    auto end_other = end(s._fragments);
    for (; end_my != it_my && end_other != it_other; ++it_my, ++it_other) {
      auto cap = min(it_my->size(), it_other->size());
      auto ret = memcmp(it_my->data(), it_other->data(), cap);
      if (ret) {
        return ret;
      }
      if (it_my->size() != it_other->size()) {
        break;
      }
    }
    return make_signed_t< usize >(_size - s.size());
  }

  string fragment_string::copy_to_string(usize sz) const {
    string ret;
    for (auto&& frag : _fragments) {
      if (0 == sz) {
        break;
      }
      auto cap = min(frag.size(), sz);
      ret.append(reinterpret_cast< char* >(frag.data()), cap);
      sz -= cap;
    }
    return move(ret);
  }
}
}
