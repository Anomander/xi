#pragma once

#include "ext/Configure.h"
#include "io/pipeline/PipelineHandler.h"

namespace xi {
namespace io {
namespace pipeline {

template < class E > void EventHandlerBase< E >::handleEvent(mut< HandlerContext > cx, own< E > e) {
  cx->forward(move(e));
}

}
}
}
