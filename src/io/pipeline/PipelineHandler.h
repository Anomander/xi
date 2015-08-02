#pragma once

#include "ext/Configure.h"
#include "io/pipeline/events/All.h"

namespace xi {
namespace io {
  namespace pipeline {

    class HandlerContext;

    template < class E >
    struct EventHandlerBase {
      virtual void handleEvent(mut< HandlerContext > cx, own< E > e);

    protected:
      void onEvent(mut< HandlerContext > cx, own< E > e) {
        this->handleEvent(cx, std::move(e));
      }
    };

    class PipelineHandler : virtual public ownership::StdShared,
                            protected meta::MultiInheritTemplate< EventHandlerBase, MessageRead, MessageWrite,
                                                                  ChannelRegistered, ChannelDeregistered, ChannelOpened,
                                                                  ChannelClosed, ChannelError, ChannelException > {
    public:
      template < class Event >
      void onEvent(mut< HandlerContext > cx, own< Event > e) {
        EventHandlerBase< Event >::onEvent(cx, std::move(e));
      }
    };
  }
}
}
