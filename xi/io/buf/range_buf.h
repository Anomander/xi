#pragma once
#include "xi/io/buf/buf.h"

namespace xi {
namespace io {

  class range_buf : public buf {
    mutable_byte_range _range;

  public:
    range_buf(mutable_byte_range rg)
        : buf(rg.writable_bytes(), rg.length()), _range(move(rg)) {}
    ~range_buf() = default;
  };
}
}
