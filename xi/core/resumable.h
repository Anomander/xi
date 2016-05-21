#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  class abstract_worker;

  namespace detail {
    using ready_hook_type = intrusive::list_member_hook<
        intrusive::link_mode< intrusive::auto_unlink > >;

    using block_hook_type = intrusive::list_member_hook<
        intrusive::link_mode< intrusive::auto_unlink > >;

    using sleep_hook_type = intrusive::set_member_hook<
        intrusive::link_mode< intrusive::auto_unlink > >;
  }

  class resumable {
    abstract_worker* _worker              = nullptr;
    steady_clock::time_point _wakeup_time = steady_clock::time_point::max();

  public:
    struct timepoint_less {
      bool operator()(resumable const& l, resumable const& r) noexcept {
        return l._wakeup_time < r._wakeup_time;
      }
    };

    detail::ready_hook_type ready_hook;
    detail::sleep_hook_type sleep_hook;

    enum resume_result {
      blocked,
      resume_later,
      done,
    };

    XI_CLASS_DEFAULTS(resumable, no_move, no_copy, virtual_dtor, ctor);

    virtual resume_result resume()    = 0;
    virtual void yield(resume_result) = 0;

    virtual void attach_executor(abstract_worker* e);
    virtual void detach_executor(abstract_worker* e);
    virtual void unblock();
    virtual void block();

    steady_clock::time_point wakeup_time();
    void wakeup_time(steady_clock::time_point);

    void sleep_for(milliseconds);

    void sleep_unlink() noexcept;
    bool is_sleep_linked() noexcept;
    void ready_unlink() noexcept;
    bool is_ready_linked() noexcept;
  };

  inline void resumable::sleep_unlink() noexcept {
    sleep_hook.unlink();
  }
  inline bool resumable::is_sleep_linked() noexcept {
    return sleep_hook.is_linked();
  }

  inline void resumable::ready_unlink() noexcept {
    ready_hook.unlink();
  }
  inline bool resumable::is_ready_linked() noexcept {
    return ready_hook.is_linked();
  }
}
}
