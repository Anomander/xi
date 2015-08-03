#pragma once

#include "ext/Configure.h"
#include "io/Message.h"

namespace xi {
namespace io {

  enum MessageType : uint8_t {
    kData = 1,
    kPing = 1 >> 1,
  };

  struct[[gnu::packed]] ProtocolHeader {
    union {
      struct[[gnu::packed]] {
        uint8_t version;
        MessageType type;
        uint32_t size;
        // Left for future versions and cache line friendliness
        char pad[2];
      };
      // For addressing header as a whole
      uint64_t bytes;
    };
  };
  static_assert(sizeof(ProtocolHeader) == 8, "Invalid size");

  struct ByteRange {
    uint8_t* data;
    size_t size;
  };

  struct ProtocolMessage {
    // It is important that binary structure of this
    // class remains the same.
    // Things that read data off of socket assume the
    // layout will not change.
    //////////////////////////////////
    ProtocolHeader _header;
    uint8_t _data[0];
    //////////////////////////////////

  public:
    ProtocolHeader const& header() const noexcept { return _header; }
    ByteRange readableRange() noexcept { return {_data, readableSize()}; }
    size_t readableSize() const noexcept { return _header.size; }
  };

  struct DataMessage : FastCastGroupMember< DataMessage, Message > {
  public:
    DataMessage(ProtocolMessage* msg) : _message(move(msg)) {}
    ~DataMessage() noexcept { ::free(_message); }

    ProtocolMessage* data() const noexcept { return _message; }

  private:
    ProtocolMessage* _message;
  };
}
}
