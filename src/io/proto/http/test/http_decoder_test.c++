#include "xi/io/proto/http/all.h"
#include "src/test/buffer_base.h"

using namespace xi;
using namespace xi::io;
using namespace xi::io::proto::http;

class decoder_test : public test::buffer_base {
protected:
  unique_ptr< decoder > dec;

  void set_up() override {
    reset();
  }

public:
  void decode(ref< string > in) {
    auto b = make_buffer(in);
    decode(move(b));
  }

  void decode(buffer in) {
    decode(edit(in));
  }

  void decode(mut<buffer> in) {
    dec->decode(in);
  }

  void reset() {
    dec = make_unique< decoder >(alloc);
  }
};

TEST_F(decoder_test, test_head) {
  auto b = make_buffer("HEAD /foo");
  decode(edit(b));
  b.push_back(make_buffer("/b"));
  b.push_back(make_buffer("a"));
  b.push_back(make_buffer("r HTTP/1.1\r\n"));
  b.push_back(make_buffer("content-length:1\r\n"));
  b.push_back(make_buffer("user-agent: Mozilla \r\n"));
  decode(edit(b));
}
