#pragma once

#include "xi/ext/configure.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/pipes/all.h"
#include "xi/io/proto/http2/decoder.h"

namespace xi {

namespace io {
  namespace proto {
    namespace http2 {

      class codec : public pipes::context_aware_filter< buffer >,
                    public delegate {

        decoder _decoder{this};
        own< buffer_allocator > _alloc;
        struct remote_settings {
          u32 header_table_size      = 4096;
          bool enable_push           = true;
          u32 max_concurrent_streams = 100;
          u32 initial_window_size    = (1 << 16) - 1;
          u32 max_frame_size         = 1 << 14;
          u32 max_header_list_size   = -1;
        } _remote_settings;

      public:
        codec(own< buffer_allocator > alloc) : _alloc(move(alloc)) {
        }

        void read(mut< context > cx, buffer in) final override {
          _decoder.decode(edit(in));
        }

        void setting_received(u16 id, u32 value) override {
          std::cout << "id:" << id << " value:" << value << std::endl;
          switch ((setting)id) {
            case setting::SETTINGS_HEADER_TABLE_SIZE:
              _remote_settings.header_table_size = value;
              break;
            case setting::SETTINGS_ENABLE_PUSH:
              _remote_settings.enable_push = (bool)value;
              break;
            case setting::SETTINGS_MAX_CONCURRENT_STREAMS:
              _remote_settings.max_concurrent_streams = value;
              break;
            case setting::SETTINGS_INITIAL_WINDOW_SIZE:
              _remote_settings.initial_window_size = value;
              break;
            case setting::SETTINGS_MAX_FRAME_SIZE:
              _remote_settings.max_frame_size = value;
              break;
            case setting::SETTINGS_MAX_HEADER_LIST_SIZE:
              _remote_settings.max_header_list_size = value;
              break;
            default:
              break;
          }
        }
        void settings_end() override {
          auto b = _alloc->allocate(1 << 10);
          b.write(byte_range{"\0\0\0\4\1\0\0\0\0"});
          my_context()->forward_write(move(b));
        }
        void settings_ack() override {
        }
        void send_connection_error(http2::error e) override {
          auto b = _alloc->allocate(1 << 10);
          b.write(byte_range{"\0\0\x8\7\0\0\0\0\0\0\0\0\0\0\0\0"});
          b.write(byte_range_for_object(e));
          my_context()->forward_write(move(b));
          my_context()->channel()->close();
        }
      };
    }
  }
}
}
