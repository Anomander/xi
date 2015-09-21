#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/DataMessage.h"

namespace xi {
namespace io {
  namespace pipeline {

    struct Event {};
    struct DownstreamEvent : Event {};
    struct UpstreamEvent : Event {};

    class ChannelOpened : public UpstreamEvent {};
    class ChannelClosed : public DownstreamEvent {};

    class ChannelRegistered : public UpstreamEvent {};
    class ChannelDeregistered : public UpstreamEvent {};

    class DataAvailable : public UpstreamEvent {};

    class ChannelError : public UpstreamEvent {
    public:
      ChannelError(error_code error) : _error(error) {}

      error_code error() const noexcept { return _error; }

    private:
      error_code _error;
    };

    struct UserUpstreamEvent : public UpstreamEvent {
#ifdef XI_USER_UPSTREAM_MESSAGE_ROOT
      using RootType = XI_USER_UPSTREAM_MESSAGE_ROOT;
      RootType root;
      UserUpstreamEvent(RootType r) : root(move(r)) {}
#endif // XI_USER_UPSTREAM_MESSAGE_ROOT
    };

    struct UserDownstreamEvent : public DownstreamEvent {
#ifdef XI_USER_DOWNSTREAM_MESSAGE_ROOT
      using RootType = XI_USER_DOWNSTREAM_MESSAGE_ROOT;
      RootType root;
      UserDownstreamEvent(RootType r) : root(move(r)) {}
#endif // XI_USER_DOWNSTREAM_MESSAGE_ROOT
    };

    class MessageRead : public UpstreamEvent {
    public:
      MessageRead(own< Message > msg) : _message(move(msg)) {}

      mut< Message > message() noexcept { return edit(_message); }
      own< Message > extractMessage() noexcept { return move(_message); }

    private:
      own< Message > _message;
    };

    class MessageWrite : public DownstreamEvent {
    public:
      MessageWrite(own< Message > msg) : _message(move(msg)) {}

      mut< Message > message() noexcept { return edit(_message); }
      own< Message > extractMessage() noexcept { return move(_message); }

    private:
      own< Message > _message;
    };

    class ChannelException : public UpstreamEvent {
    public:
      ChannelException(exception_ptr ex) : _exception(ex) {}

      exception_ptr exception() const noexcept { return _exception; }

    private:
      exception_ptr _exception;
    };
  }
}
}
