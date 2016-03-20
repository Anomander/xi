#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace proto {
    namespace http2 {
      static constexpr const char INITIATION_STRING[] =
          "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

      enum class state : u8 {
        READ_CONNECTION_PREFACE,
        READ_FRAME_HEADER,
        READ_FRAME_PADDING,
        READ_DATA_FRAME = 0xF0,
        READ_HEADERS_FRAME = 0xF1,
        READ_PRIORITY_FRAME = 0xF2,
        READ_RST_STREAM_FRAME = 0xF3,
        READ_SETTINGS_FRAME = 0xF4,
        READ_PUSH_PROMISE_FRAME = 0xF5,
        READ_PING_FRAME = 0xF6,
        READ_GOAWAY_FRAME = 0xF7,
        READ_WINDOW_UPDATE_FRAME = 0xF8,
        READ_CONTINUATION_FRAME = 0xF9,
        CONNECTION_ERROR = 0xFF,
      };

      enum class frame : u8 {
        DATA_FRAME = 0X00,
        HEADERS_FRAME = 0X01,
        PRIORITY_FRAME = 0X02,
        RST_STREAM_FRAME = 0X03,
        SETTINGS_FRAME = 0X04,
        PUSH_PROMISE_FRAME = 0X05,
        PING_FRAME = 0X06,
        GOAWAY_FRAME = 0X07,
        WINDOW_UPDATE_FRAME = 0X08,
        CONTINUATION_FRAME = 0X09,
        MAX_KNOWN_FRAME = CONTINUATION_FRAME,
        INVALID_FRAME_TYPE = 0XFF,
      };

      enum class flag : u8 {
        ACK = 0x01,
        END_STREAM = 0x01,
        END_SEGMENT = 0x02,
        END_HEADERS = 0x04,
        PADDED = 0x08,
        PRIORITY = 0x20,
      };

      enum class setting : u16 {
        SETTINGS_HEADER_TABLE_SIZE = 0x1,
        SETTINGS_ENABLE_PUSH = 0x2,
        SETTINGS_MAX_CONCURRENT_STREAMS = 0x3,
        SETTINGS_INITIAL_WINDOW_SIZE = 0x4,
        SETTINGS_MAX_FRAME_SIZE = 0x5,
        SETTINGS_MAX_HEADER_LIST_SIZE = 0x6,
        SETTINGS_MAX
      };

      enum class error : u8 {
        NO_ERROR = 0x0, // The associated condition is not a result of an error.
        PROTOCOL_ERROR = 0x1, // The endpoint detected an unspecific protocol
                              // error.
        INTERNAL_ERROR = 0x2, // The endpoint encountered an unexpected internal
                              // error.
        FLOW_CONTROL_ERROR = 0x3, // The endpoint detected that its peer
                                  // violated the
                                  // flow-control protocol.
        SETTINGS_TIMEOUT = 0x4,   // The endpoint sent a SETTINGS frame but did
                                  // not
                                  // receive a response in a timely manner.
        STREAM_CLOSED = 0x5, // The endpoint received a frame after a stream was
                             // half-closed.
        FRAME_SIZE_ERROR = 0x6, // The endpoint received a frame with an invalid
                                // size.
        REFUSED_STREAM = 0x7,   // The endpoint refused the stream prior to
                                // performing
                                // any application processing.
        CANCEL = 0x8, // Used by the endpoint to indicate that the stream is no
                      // longer
                      // needed.
        COMPRESSION_ERROR = 0x9, // The endpoint is unable to maintain the
                                 // header
                                 // compression context for the connection.
        CONNECT_ERROR = 0xa,     // The connection established in response to a
                                 // CONNECT
        // request (Section 8.3) was reset or abnormally closed.
        ENHANCE_YOUR_CALM = 0xb, // The endpoint detected that its peer is
        // exhibiting a behavior that might be generating excessive load.
        INADEQUATE_SECURITY = 0xc, // The underlying transport has properties
        // that do not meet minimum security requirements (see Section 9.2).
        HTTP_1_1_REQUIRED = 0xd, // The endpoint requires that HTTP/1.1 be used
                                 // instead of HTTP/2.
      };

      enum : u32 {
        FRAME_HEADER_SIZE = 9,
        CONNECTION_PREFACE_LENGTH = sizeof(INITIATION_STRING) - 1,
        MAX_ALLOWED_FRAME_SIZE = (1u << 24) - 1,
        MAX_ALLOWED_WINDOW_SIZE = (1u << 31) - 1,
      };
      static_assert(CONNECTION_PREFACE_LENGTH == 24, "");
    }
  }
}
}
