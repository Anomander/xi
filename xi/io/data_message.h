#pragma once

#include "xi/ext/configure.h"
#include "xi/io/message.h"
#include "xi/io/byte_range.h"

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

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
    byte_range readable_range() noexcept { return {_data, readable_size()}; }
    size_t readable_size() const noexcept { return _header.size; }
  };

  struct data_message : fast_cast_group_member< data_message, message > {
  public:
    data_message(protocol_message *msg) : _message(move(msg)) {}
    ~data_message() noexcept { ::free(_message); }

    protocol_message *data() const noexcept { return _message; }

  private:
    protocol_message *_message;
  };
}
}
