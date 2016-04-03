#include "xi/io/proto/http2/all.h"
#include "src/test/buffer_base.h"

using namespace xi;
using namespace xi::io;
using namespace xi::io::proto::http2;

struct mock_http2_delegate : public delegate {
  vector< pair< u16, u32 > > settings;
  bool settings_finished = false;
  bool settings_acked    = false;
  error connection_error = error::NO_ERROR;

  auto& last_setting() const {
    return settings.back();
  }

  bool settings_empty() const {
    return settings.empty();
  }

private:
  void settings_ack() override {
    settings_acked = true;
  }
  void setting_received(u16 id, u32 value) override {
    settings.push_back(make_pair(id, value));
  }
  void settings_end() override {
    settings_finished = true;
  }
  void send_connection_error(error e) override {
    connection_error = e;
  }
};

class decoder_test : public test::buffer_base {
protected:
  unique_ptr< decoder > dec;
  unique_ptr< mock_http2_delegate > delegate;

  void set_up() override {
    reset();
  }

public:
  void decode(ref< string > in) {
    decode(make_buffer(in));
  }

  void decode(own<buffer> in) {
    dec->decode(edit(in));
  }

  void reset() {
    delegate = make_unique< mock_http2_delegate >();
    dec      = make_unique< decoder >(edit(delegate));
  }

  template < class T >
  void clean_decode(T&& in) {
    reset();
    decode(forward< T >(in));
  }

  template < class T >
  void new_conn_decode(T&& in) {
    reset();
    decode_preface();
    decode(forward< T >(in));
  }

  void decode_preface() {
    decode("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");
  }

  template < class I >
  void write_be(I value, mut< buffer > in, u8 bits = sizeof(I) * 8) {
    union {
      u8 byte[sizeof(I)];
      I val;
    } bytes;
    bytes.val = value;

    switch (bits / 8) {
      case 8:
        in->write(byte_range_for_object(bytes.byte[7]));
      case 7:
        in->write(byte_range_for_object(bytes.byte[6]));
      case 6:
        in->write(byte_range_for_object(bytes.byte[5]));
      case 5:
        in->write(byte_range_for_object(bytes.byte[4]));
      case 4:
        in->write(byte_range_for_object(bytes.byte[3]));
      case 3:
        in->write(byte_range_for_object(bytes.byte[2]));
      case 2:
        in->write(byte_range_for_object(bytes.byte[1]));
      case 1:
        in->write(byte_range_for_object(bytes.byte[0]));
    }
  }

  auto construct_frame(u32 stream, frame type, u8 flags, u32 length) {
    auto b = alloc->allocate(length + 9 + 100 /* for various manipulations */);
    write_be(length, edit(b), 24);
    b->write(byte_range_for_object(type));
    b->write(byte_range_for_object(flags));
    write_be(stream, edit(b));
    return move(b);
  }

  auto make_advanced_settings_frame(
      u32 stream,
      bool ack,
      vector< pair< setting, u32 > > settings = {},
      u32 length_override = 0) {
    auto f = construct_frame(stream,
                             frame::SETTINGS_FRAME,
                             ack ? 1 : 0,
                             length_override ? length_override
                                             : settings.size() * 6);
    for (auto& s : settings) {
      write_be((u16)s.first, edit(f));
      write_be(s.second, edit(f));
    }
    return move(f);
  }

  auto make_simple_settings_frame(vector< pair< setting, u32 > > settings) {
    return make_advanced_settings_frame(0, false, move(settings));
  }
};

TEST_F(decoder_test, client_preface_is_invalid) {
  decode("PIR * HTTP/2.0\r\n\r\nSM\r\n\r\n"); // invalid verb
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);

  clean_decode("* HTTP/2.0\r\n\r\nSM\r\n\r\n"); // verb missing
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);

  clean_decode("PRI * HTTP/1.1\r\n\r\nSM\r\n\r\n"); // invalid protocol
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);

  clean_decode("PRI * HTTP/2.0\r\n\r\n\r\n\r\n"); // body missing
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);

  clean_decode("PRI * HTTP/2.0\n\n\n\nSM\n\n\n\n"); // s/\r/\n/g
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);

  clean_decode("PRI * HTTP/2.0\n\nSM\n\n"); // s/\r//g
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, client_preface_is_valid) {
  decode("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");
  EXPECT_EQ(error::NO_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_frame_errors) {
  // ack + length
  new_conn_decode(make_advanced_settings_frame(
      0, true, {{setting::SETTINGS_ENABLE_PUSH, 1}}));
  EXPECT_EQ(error::FRAME_SIZE_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_frame_errors_incoherent_length) {
  // settings should be a multiple of 6 bytes in length
  auto frame = make_advanced_settings_frame(0, false, {}, 4);
  frame->write(byte_range_for_object((u32)1));
  new_conn_decode(move(frame));
  EXPECT_EQ(error::FRAME_SIZE_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_frame_errors_stream_id_not_0) {
  new_conn_decode(make_advanced_settings_frame(1, true));
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);

  new_conn_decode(make_advanced_settings_frame(1, false));
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);

  new_conn_decode(make_advanced_settings_frame(
      1, false, {{setting::SETTINGS_ENABLE_PUSH, 1}}));
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_frame_errors_ack_has_length) {
  new_conn_decode(make_advanced_settings_frame(
      0, true, {{setting::SETTINGS_ENABLE_PUSH, 1}}));
  EXPECT_EQ(error::FRAME_SIZE_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_value_errors_push) {
  // enable push > 1
  new_conn_decode(
      make_simple_settings_frame({{setting::SETTINGS_ENABLE_PUSH, 2}}));
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);
  // enable push >>> 1
  new_conn_decode(
      make_simple_settings_frame({{setting::SETTINGS_ENABLE_PUSH, -1}}));
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_value_errors_initial_window_size) {
  // max window size is too large
  new_conn_decode(make_simple_settings_frame(
      {{setting::SETTINGS_INITIAL_WINDOW_SIZE, MAX_ALLOWED_WINDOW_SIZE + 1}}));
  EXPECT_EQ(error::FLOW_CONTROL_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_value_errors_max_frame_size) {
  // max frame size is too large
  new_conn_decode(make_simple_settings_frame(
      {{setting::SETTINGS_MAX_FRAME_SIZE, MAX_ALLOWED_FRAME_SIZE + 1}}));
  EXPECT_EQ(error::PROTOCOL_ERROR, delegate->connection_error);
}

TEST_F(decoder_test, settings_value_is_propagated) {
  auto check_setting = [&](auto s, u32 v) {
    decode(make_simple_settings_frame({{s, v}}));

    EXPECT_EQ(error::NO_ERROR, delegate->connection_error);
    EXPECT_EQ((u16)s, delegate->last_setting().first);
    EXPECT_EQ(v, delegate->last_setting().second);
  };

  decode_preface();

  check_setting(setting::SETTINGS_HEADER_TABLE_SIZE, 1024);
  check_setting(setting::SETTINGS_ENABLE_PUSH, 0);
  check_setting(setting::SETTINGS_ENABLE_PUSH, 1);
  check_setting(setting::SETTINGS_MAX_CONCURRENT_STREAMS, 1);
  check_setting(setting::SETTINGS_MAX_CONCURRENT_STREAMS, 100);
  check_setting(setting::SETTINGS_INITIAL_WINDOW_SIZE, 100);
  check_setting(setting::SETTINGS_MAX_FRAME_SIZE, 1);
  check_setting(setting::SETTINGS_MAX_FRAME_SIZE, 1 << 20);
  check_setting(setting::SETTINGS_MAX_HEADER_LIST_SIZE, 1);
  check_setting(setting::SETTINGS_MAX_HEADER_LIST_SIZE, 1 << 20);
}

TEST_F(decoder_test, settings_ack_is_propagated) {
  decode_preface();

  new_conn_decode(make_advanced_settings_frame(0, true));
  EXPECT_EQ(error::NO_ERROR, delegate->connection_error);
  EXPECT_TRUE(delegate->settings_acked);
}

TEST_F(decoder_test, unknown_settings_are_ignored) {
  decode_preface();

  decode(make_simple_settings_frame({{setting::SETTINGS_MAX, 1}}));
  EXPECT_TRUE(delegate->settings_empty());

  for (auto i = 3; i < 15; ++i) {
    decode(make_simple_settings_frame({{(setting)(1 << i), 1024}}));
    EXPECT_TRUE(delegate->settings_empty());
  }
}
