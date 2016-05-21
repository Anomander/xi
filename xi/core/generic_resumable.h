#pragma once

#include "xi/ext/coroutine.h"
#include "xi/core/resumable.h"

namespace xi {
namespace core {

  class generic_resumable : public resumable {
    using coro_t    = symmetric_coroutine< void >::call_type;
    using yield_t   = symmetric_coroutine< void >::yield_type;
    using context_t = ext::execution_context;

    // context_t _yield_ctx;
    // context_t _my_ctx;
    coro_t _coro;
    yield_t* _yield       = nullptr;
    resume_result _result = resume_later;
    bool _is_running      = false;

  protected:
    resume_result resume() final override {
      if (!_is_running) {
        _is_running = true;
        _coro();
      }
      return _result;
    }

    // resume_result resume() final override {
    //   printf("Resuming from %p.\n", pthread_self());
    //   if (!_is_running) {
    //     _is_running = true;
    //     return *reinterpret_cast< resume_result* >(_my_ctx());
    //   }
    //   return resume_later;
    // }

    void yield(resume_result result) final override {
      if (_is_running) {
        _is_running = false;
        _result     = result;
        (*_yield)();
      }
    }

    // void yield(resume_result result) final override {
    //   if (_is_running) {
    //     _is_running = false;
    //     _result     = result;
    //     _yield_ctx(&_result);
    //   }
    // }

    virtual void call() = 0;

    generic_resumable()
        :
      // _yield_ctx(context_t::current())
      //   , _my_ctx([this](void*) {
      //     _is_running = true;
      //     call();
      //     _result = done;
      //     _yield_ctx(&_result);
      //   })
        // ,
      _coro(
              [this](yield_t& y) {
                _yield      = &y;
                _is_running = true;
                call();
                _result = done;
              },
              attributes(),
              standard_stack_allocator()) {
      // printf("Created in %p : %p.\n", pthread_self(), this);
      // printf("%s %p\n", __PRETTY_FUNCTION__, this);
    }

    ~generic_resumable() {
      printf("%s\n", __PRETTY_FUNCTION__);
    }

    XI_CLASS_DEFAULTS(generic_resumable, no_move, no_copy);
  };
}
}
