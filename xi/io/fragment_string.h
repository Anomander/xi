#pragma once

#include "xi/io/fragment.h"
#include "xi/ext/string.h"

namespace xi {
namespace io {

  class fragment_string final {
    using list_t =
        intrusive::list< fragment,
                         intrusive::link_mode< intrusive::normal_link >,
                         intrusive::constant_time_size< true > >;
    list_t _fragments;
    usize _size = 0;

  public:
    fragment_string(list_t &&, usize);
    fragment_string() = default;
    fragment_string(own< fragment >);
    ~fragment_string();
    XI_CLASS_DEFAULTS(fragment_string, move);

    bool empty() const;
    usize length() const;
    usize size() const;

    i32 compare(string_ref) const;
    i32 compare(ref<fragment_string>) const;
    string copy_to_string(usize = -1) const;
  };

  inline bool fragment_string::empty() const {
    return _fragments.empty();
  }

  inline usize fragment_string::length() const {
    return _fragments.size();
  }

  inline usize fragment_string::size() const {
    return _size;
  }

  inline bool operator< (ref<fragment_string> l, ref<fragment_string> r) {
    return l.compare(r) < 0;
  }

  inline bool operator> (ref<fragment_string> l, ref<fragment_string> r) {
    return l.compare(r) < 0;
  }

  inline bool operator== (ref<fragment_string> l, ref<fragment_string> r) {
    return l.compare(r) == 0;
  }

  inline bool operator== (ref<fragment_string> l, string_ref r) {
    return l.compare(r) == 0;
  }

  inline bool operator== (string_ref l, ref<fragment_string> r) {
    return r == l;
  }
}
}
