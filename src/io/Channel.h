#pragma once

#include "ext/Configure.h"
#include "io/Message.h"

namespace xi {
  namespace io {

    class Channel
      : virtual public ownership::StdShared
    {
    protected:
      virtual ~Channel() noexcept = default;
    public:
      virtual void close() = 0;
      virtual void write(own<Message>) = 0;
    };

    class ClientChannelConnected
      : public FastCastGroupMember <ClientChannelConnected, Message>
    {
    public:
      ClientChannelConnected (own<Channel> ch)
        : _channel (move(ch))
      {}

      mut<Channel> channelRef() { return edit(_channel); }
      own<Channel> extractChannel() { return move(_channel); }

    private:
      own<Channel> _channel;
    };

  }
}
