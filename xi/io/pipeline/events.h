#pragma once

#include "xi/ext/configure.h"
#include "xi/io/data_message.h"

namespace xi {
namespace io {
  namespace pipeline {

    struct event {};
    struct downstream_event : event {};
    struct upstream_event : event {};

    class channel_opened : public upstream_event {};
    class channel_closed : public downstream_event {};

    class channel_registered : public upstream_event {};
    class channel_deregistered : public upstream_event {};

    class data_available : public upstream_event {};

    class channel_error : public upstream_event {
    public:
      channel_error(error_code error) : _error(error) {}

      error_code error() const noexcept { return _error; }

    private:
      error_code _error;
    };

    struct user_upstream_event : public upstream_event {
#ifdef XI_USER_UPSTREAM_MESSAGE_ROOT
      using root_type = XI_USER_UPSTREAM_MESSAGE_ROOT;
      root_type root;
      user_upstream_event(root_type r) : root(move(r)) {}
#endif // XI_USER_UPSTREAM_MESSAGE_ROOT
    };

    struct user_downstream_event : public downstream_event {
#ifdef XI_USER_DOWNSTREAM_MESSAGE_ROOT
      using root_type = XI_USER_DOWNSTREAM_MESSAGE_ROOT;
      root_type root;
      user_downstream_event(root_type r) : root(move(r)) {}
#endif // XI_USER_DOWNSTREAM_MESSAGE_ROOT
    };

    class message_read : public upstream_event {
    public:
      message_read(own< message > msg) : _message(move(msg)) {}

      mut< message > msg() noexcept { return edit(_message); }
      own< message > extract_message() noexcept { return move(_message); }

    private:
      own< message > _message;
    };

    class message_write : public downstream_event {
    public:
      message_write(own< message > msg) : _message(move(msg)) {}

      mut< message > msg() noexcept { return edit(_message); }
      own< message > extract_message() noexcept { return move(_message); }

    private:
      own< message > _message;
    };

    class channel_exception : public upstream_event {
    public:
      channel_exception(exception_ptr ex) : _exception(ex) {}

      exception_ptr exception() const noexcept { return _exception; }

    private:
      exception_ptr _exception;
    };
  }
}
}
