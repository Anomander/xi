#pragma once

#include "xi/ext/configure.h"
#include "xi/io/byte_range.h"

namespace xi {
namespace io {

  enum message_type : uint8_t {
    kData = 1,
    kPing = 1 >> 1,
  };

  struct[[gnu::packed]] protocol_header {
    union {
      struct[[gnu::packed]] {
        uint8_t version;
        message_type type;
        uint32_t size;
        // left for future versions
        char pad[2];
      };
      // for addressing header as a whole
      uint64_t bytes;
    };
  };
  static_assert(sizeof(protocol_header) == 8, "Invalid size");

  struct protocol_message
      : public boost::intrusive_ref_counter< protocol_message,
                                             boost::thread_unsafe_counter > {
    // it is important that binary structure of this
    // class remains the same.
    // things that read data off of socket assume the
    // layout will not change.
    //////////////////////////////////
    protocol_header _header;
    uint8_t _data[0];
    //////////////////////////////////

  public:
    protocol_header const &header() const noexcept { return _header; }
    byte_range readable_range() noexcept { return byte_range{_data, readable_size()}; }
    size_t readable_size() const noexcept { return _header.size; }
  };

}
}
