#pragma once

#include "xi/ext/configure.h"
#include "xi/core/resumable.h"

namespace xi {
namespace core {

  class basic_resumable : public resumable {
    void attach_executor(execution_context*) override {}
    void detach_executor(execution_context*) override {}
    resume_result resume() override {}
  };

}
}
