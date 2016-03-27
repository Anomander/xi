#pragma once

#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/buffer_reader.h"

namespace xi {
namespace io {
  namespace proto {
    namespace http {
      enum class verb { GET, HEAD, PUT, POST, DELETE, INVALID };
      enum class state { VERB, URI, VERSION, CRLF, HEADERS, BODY };

      class decoder {
        state _state  = state::VERB;
        verb _verb    = verb::INVALID;
        usize _length = 0;
        own< buffer_allocator > _alloc;
        string _uri = {};
        u8 _version = -1;

      public:
        decoder(own< buffer_allocator > alloc) : _alloc(move(alloc)) {
        }

        void decode(mut< buffer > in) {
          auto loop = true;
          while (loop) {
            std::cout << in->size() << " bytes remaining" << std::endl;
            switch (_state) {
              case state::VERB:
                std::cout << "Reading VERB" << std::endl;
                loop = read_verb(in);
                break;
              case state::URI:
                std::cout << "Reading URI" << std::endl;
                loop = read_uri(in);
                break;
              case state::VERSION:
                std::cout << "Reading VERSION" << std::endl;
                loop = read_version(in);
                break;
              case state::CRLF:
                std::cout << "Reading CRLF" << std::endl;
                loop = read_crlf(in);
                break;
              case state::HEADERS:
                std::cout << "Reading HEADERS" << std::endl;
                loop = false;
                break;
              case state::BODY:
                std::cout << "Reading BODY" << std::endl;
                loop = false;
                break;
              default:
                break;
            }
          }
        }

      private:
        bool read_verb(mut< buffer > in) {
          if (in->size() < 6) {
            return false;
          }
          auto r = make_consuming_reader(in);
          switch (r.read< u8 >().unwrap()) {
            case 'D':
              if (compare_bytes("ELETE", in)) {
                _verb = verb::DELETE;
                std::cout << "DELETE" << std::endl;
              }
              break;
            case 'H':
              if (compare_bytes("EAD", in)) {
                _verb = verb::HEAD;
                std::cout << "HEAD" << std::endl;
              }
              break;
            case 'G':
              if (compare_bytes("ET", in)) {
                _verb = verb::GET;
                std::cout << "GET" << std::endl;
              }
              break;
            case 'P':
              if (compare_bytes("UT", in)) {
                _verb = verb::PUT;
                std::cout << "PUT" << std::endl;
              } else if (compare_bytes("OST", in)) {
                _verb = verb::POST;
                std::cout << "POST" << std::endl;
              }
              break;
          }
          if (_verb != verb::INVALID) {
            in->skip_bytes(1); // skip space
            _state = state::URI;
          }
          return in->size() > 0;
        }

        bool read_uri(mut< buffer > in) {
          auto r = make_consuming_reader(in);
          if (auto space_pos = r.find_byte(' ', _length)) {
            _length = space_pos.unwrap();
            r.append_to_string(edit(_uri), _length);
            std::cout << "uri length " << _length << std::endl;
            std::cout << "uri [" << _uri << "]" << std::endl;
            in->skip_bytes(1); // skip space
            _state = state::VERSION;
          } else {
            r.append_to_string(edit(_uri));
            return false;
          }
          return in->size() > 0;
        }

        bool read_version(mut< buffer > in) {
          if (in->size() < 8) {
            return false;
          }
          auto r = make_consuming_reader(in);
          if ('H' == r.read< char >().unwrap()) {
            if (compare_bytes("TTP/1.", in)) {
              _version = r.read< u8 >().unwrap() - '0';
              std::cout << "VERSION is HTTP/1." << (u32)_version << std::endl;
              _state = state::CRLF;
            }
          }
          return in->size() > 0;
        }
        bool read_crlf(mut< buffer > in) {
          if (in->size() < 2) {
            return false;
          }
          if (compare_bytes("\r\n", in)) {
            _state = state::HEADERS;
          }
          return in->size() > 0;
        }

        template < u32 N >
        bool compare_bytes(const char (&pattern)[N], mut< buffer > in) {
          u32 i  = 0;
          auto r = make_consuming_reader(in);
          while (auto b = r.peek< char >()) {
            if (b.unwrap() != pattern[i++]) {
              return false;
            }
            in->skip_bytes(1);
            if (i == N - 1) {
              break;
            }
          }
          return true;
        }
      };
    }
  }
}
}
