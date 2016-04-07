#pragma once

#include "xi/ext/configure.h"
#include "xi/io/byte_range.h"
#include "xi/io/byte_blob.h"

namespace xi {
namespace io {

  class fragment : public virtual ownership::unique,
                   public intrusive::list_base_hook<
                       intrusive::link_mode< intrusive::normal_link > > {
    enum internal_t { INTERNAL };

  private:
    byte_blob _bytes;
    u8* _head = _bytes.begin();
    usize _size = 0;

  public:
    fragment(byte_blob) noexcept;

  private:
    XI_CLASS_DEFAULTS(fragment, no_move, no_copy);

    void maybe_release();

  public:
    u8* data();
    u8 const* data() const;
    u8* tail() const;
    usize headroom() const;
    usize size() const;
    usize tailroom() const;
    bool empty() const;

    byte_range head_range();
    const byte_range data_range() const;
    byte_range tail_range();

    void advance(usize);
    void retreat(usize);

    usize write(const byte_range);
    usize read(byte_range, usize offset = 0);
    usize skip_bytes(usize len);
    usize ignore_bytes_at_end(usize len);

    unique_ptr< fragment > clone();
    unique_ptr< fragment > split(usize sz);

    usize record_bytes(usize);
  };
  static_assert(sizeof(fragment) <= 72, "Unexpected growth");

  inline u8* fragment::data() {
    return _head;
  }
  inline u8 const* fragment::data() const {
    return _head;
  }
  inline u8* fragment::tail() const {
    return _head + _size;
  }
  inline usize fragment::headroom() const {
    return _head - _bytes.begin();
  }
  inline usize fragment::size() const {
    return _size;
  }
  inline bool fragment::empty() const {
    return 0 == size();
  }
  inline usize fragment::tailroom() const {
    return _bytes.end() - tail();
  }
  inline byte_range fragment::head_range() {
    return byte_range{_head, headroom()};
  }
  inline const byte_range fragment::data_range() const {
    return byte_range{_head, size()};
  }
  inline byte_range fragment::tail_range() {
    return byte_range{tail(), tailroom()};
  }
  inline usize fragment::record_bytes(usize sz) {
    assert(make_signed_t< usize >(sz + size()) >= 0);
    auto increment = min(sz, tailroom());
    _size += increment;
    return increment;
  }
}
}
