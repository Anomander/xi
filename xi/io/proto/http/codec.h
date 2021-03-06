#pragma once

#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/pipes/all.h"
#include "xi/io/proto/http/decoder.h"

namespace xi {
namespace io {
  namespace proto {
    namespace http {

      class codec : public pipes::context_aware_filter< own<buffer> > {
        decoder _decoder;
        buffer _read_buf;

      private:
        void read(mut< context > cx, own<buffer> in) final override {
          _read_buf.push_back(move(in));
          _decoder.decode(edit(_read_buf));
        }
      };
    }
  }
}
}
