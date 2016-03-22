#pragma once

#include "xi/ext/configure.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_reader.h"
#include "xi/io/proto/http2/constants.h"

namespace xi {

namespace io {
  namespace proto {
    namespace http2 {

      struct delegate {
        virtual ~delegate() = default;
        virtual void setting_received(u16 id, u32 value) = 0;
        virtual void settings_end()                        = 0;
        virtual void settings_ack()                        = 0;
        virtual void send_connection_error(http2::error e) = 0;
      };

      class decoder {
        u32 _length               = 0;
        frame _type               = frame::INVALID_FRAME_TYPE;
        state _state              = state::READ_CONNECTION_PREFACE;
        u8 _flags                 = 0;
        u8 _padding               = 0;
        u32 _stream_id            = 0;
        mut< delegate > _delegate = nullptr;

      public:
        decoder(mut< delegate > delegate) : _delegate(delegate) {
          assert(_delegate != nullptr);
        }

        void decode(mut< buffer > in) {
          std::cout << "Decoder state: " << (int)_state << std::endl;
          bool loop = true;
          while (loop) {
            std::cout << "Remaining size: " << in->size() << std::endl;
            switch (_state) {
              case http2::state::READ_CONNECTION_PREFACE:
                loop = read_connection_preface(in);
                break;
              case http2::state::READ_FRAME_HEADER:
                loop = read_frame_header(in);
                break;
              case http2::state::READ_FRAME_PADDING:
                loop = read_frame_padding(in);
                break;
              case http2::state::READ_DATA_FRAME:
                loop = read_data_frame(in);
                break;
              case http2::state::READ_HEADERS_FRAME:
                std::cout << "Reading HEADERS" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_PRIORITY_FRAME:
                std::cout << "Reading PRIORITY" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_RST_STREAM_FRAME:
                std::cout << "Reading RST_STREAM" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_SETTINGS_FRAME:
                std::cout << "Reading SETTINGS" << std::endl;
                loop   = read_settings_frame(in);
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_PUSH_PROMISE_FRAME:
                std::cout << "Reading PUSH_PROMISE" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_PING_FRAME:
                std::cout << "Reading PING" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_GOAWAY_FRAME:
                std::cout << "Reading GOAWAY" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_WINDOW_UPDATE_FRAME:
                std::cout << "Reading WINDOW_UPDATE" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::READ_CONTINUATION_FRAME:
                std::cout << "Reading CONTINUATION" << std::endl;
                in->skip_bytes(_length);
                loop   = in->size() > _length;
                _state = http2::state::READ_FRAME_HEADER;
                break;
              case http2::state::CONNECTION_ERROR:
                std::cout << "Connection error" << std::endl;
                return;
            }
          }
        }

      private:
        bool read_connection_preface(mut< buffer > in) {
          auto r = make_consuming_reader(in);
          while (auto b = r.read< char >()) {
            if (b.unwrap() != INITIATION_STRING[_length++]) {
              _state = http2::state::CONNECTION_ERROR;
              _delegate->send_connection_error(http2::error::PROTOCOL_ERROR);
              return false;
            }
            if (_length == CONNECTION_PREFACE_LENGTH) {
              std::cout << "HTTP2 Yay!" << std::endl;
              _state = http2::state::READ_FRAME_HEADER;
              return false;
            }
          }
          return true;
        }

        bool read_frame_padding(mut< buffer > in) {
          auto r = make_consuming_reader(in);
          if (auto p = r.read< u8 >()) {
            _padding = p.unwrap();
            _state   = get_state_from_type();
            --_length;
          }
          return true;
        }

        bool read_settings_frame(mut< buffer > in) {
          auto ack = has_flag(flag::ACK);
          if (_length == 0) {
            if (ack) {
              _delegate->settings_ack();
            }
            _state = http2::state::READ_FRAME_HEADER;
            return false;
          }
          if (ack) {
            transition_to_error(http2::error::FRAME_SIZE_ERROR);
            return false;
          }
          if (_length % 6) {
            transition_to_error(http2::error::PROTOCOL_ERROR);
            return false;
          }
          if (_stream_id) {
            transition_to_error(http2::error::PROTOCOL_ERROR);
            return false;
          }
          while (_length && in->size() >= 6) {
            auto id    = read_u16(in);
            auto value = read_u32(in);
            std::cout << "id:" << id << " value:" << value << std::endl;
            if (id >= (u16)setting::SETTINGS_MAX) {
              std::cout << "Ignoring unknown setting" << std::endl;
              continue;
            }
            switch ((setting)id) {
              case setting::SETTINGS_ENABLE_PUSH:
                if (value > 1) {
                  transition_to_error(http2::error::PROTOCOL_ERROR);
                  return false;
                }
                break;
              case setting::SETTINGS_INITIAL_WINDOW_SIZE:
                if (value > MAX_ALLOWED_WINDOW_SIZE) {
                  transition_to_error(http2::error::FLOW_CONTROL_ERROR);
                  return false;
                }
                break;
              case setting::SETTINGS_MAX_FRAME_SIZE:
                if (value > MAX_ALLOWED_FRAME_SIZE) {
                  transition_to_error(http2::error::PROTOCOL_ERROR);
                  return false;
                }
                break;
              default:
                break;
            }
            _delegate->setting_received(id, value);
            _length -= 6;
          }
          if (0 == _length) {
            _delegate->settings_end();
          }
          return in->size() > _length;
        }

        bool read_data_frame(mut< buffer > in) {
          std::cout << "Reading DATA" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          return in->size() > 0;
        }

        u16 read_u16(mut< buffer > in) {
          auto r = make_consuming_reader(in);
          struct _u16 {
            u8 bytes[2];
          } val = r.read< _u16 >().unwrap();

          return val.bytes[0] << 8 | val.bytes[1];
        }

        u32 read_u24(mut< buffer > in) {
          auto r = make_consuming_reader(in);
          struct _u24 {
            u8 bytes[3];
          } val = r.read< _u24 >().unwrap();

          return val.bytes[0] << 16 | val.bytes[1] << 8 | val.bytes[2];
        }

        u32 read_u32(mut< buffer > in) {
          auto r = make_consuming_reader(in);
          struct _u32 {
            u8 bytes[4];
          } val = r.read< _u32 >().unwrap();

          return val.bytes[0] << 24 | val.bytes[1] << 16 | val.bytes[2] << 8 |
                 val.bytes[3];
        }

        bool read_frame_header(mut< buffer > in) {
          if (in->size() < FRAME_HEADER_SIZE) {
            return false;
          }

          auto r = make_consuming_reader(in);

          _length = read_u24(in);
          std::cout << "Length: " << _length << std::endl;
          _type = r.read< frame >().unwrap();
          std::cout << "Type: " << (int)_type << std::endl;
          _flags = r.read< u8 >().unwrap();
          std::cout << "Flags: " << (int)_flags << std::endl;
          _stream_id = read_u32(in);
          std::cout << "Stream: " << _stream_id << std::endl;

          error err = validate_frame_header();
          if (err != error::NO_ERROR) {
            transition_to_error(err);
            return false;
          }

          _state = has_flag(flag::PADDED) ? http2::state::READ_FRAME_PADDING
                                          : get_state_from_type();
          return true; // always try to process the frame
        }

        bool has_flag(flag f) {
          return _flags & static_cast< u8 >(f);
        }

        http2::state get_state_from_type() {
          return static_cast< http2::state >(static_cast< u8 >(_type) | 0xF0);
        }

        void transition_to_error(http2::error e) {
          _state = http2::state::CONNECTION_ERROR;
          _delegate->send_connection_error(e);
        }

        bool is_stream_id_valid() {
          switch (_type) {
            case frame::DATA_FRAME:
              return 0 != _stream_id;
            case frame::HEADERS_FRAME:
              return 0 != _stream_id;
            case frame::PRIORITY_FRAME:
              return _stream_id != 0;
            case frame::RST_STREAM_FRAME:
              return _stream_id != 0;
            case frame::SETTINGS_FRAME:
              return 0 == _stream_id;
            case frame::PUSH_PROMISE_FRAME:
              return 0 != _stream_id;
            case frame::PING_FRAME:
              return _stream_id == 0;
            case frame::GOAWAY_FRAME:
              return _stream_id == 0;
            default:
              return true;
          }
        }

        bool is_length_valid() {
          switch (_type) {
            case frame::DATA_FRAME:
              return _length >= has_flag(flag::PADDED) ? 1 : 0;
            case frame::HEADERS_FRAME: {
              u32 min_length = has_flag(flag::PADDED) ? 1 : 0;
              if (has_flag(flag::PRIORITY)) {
                min_length += 5;
              }
              return _length >= min_length;
            }
            case frame::PRIORITY_FRAME:
              return _length == 5;
            case frame::RST_STREAM_FRAME:
              return _length == 4;
            case frame::SETTINGS_FRAME:
              return has_flag(flag::ACK) ? _length == 0 : (_length % 6) == 0;
            case frame::PUSH_PROMISE_FRAME:
              return _length >= has_flag(flag::PADDED) ? 5 : 4;
            case frame::PING_FRAME:
              return _length == 8;
            case frame::GOAWAY_FRAME:
              return _length >= 8;
            case frame::WINDOW_UPDATE_FRAME:
              return _length == 4;
            default:
              return true;
          }
        }

        error validate_frame_header() {
          if (_length >= MAX_ALLOWED_FRAME_SIZE) {
            return error::FRAME_SIZE_ERROR;
          }
          if (_type > frame::MAX_KNOWN_FRAME) {
            return error::PROTOCOL_ERROR;
          }
          if (!is_stream_id_valid()) {
            return error::PROTOCOL_ERROR;
          }
          if (!is_length_valid()) {
            return error::FRAME_SIZE_ERROR;
          }
          std::cout << "frame is valid" << std::endl;
          return error::NO_ERROR;
        }
      };
    }
  }
}
}
