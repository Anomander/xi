#pragma once

#include "xi/ext/coroutine.h"
#include "xi/core/resumable.h"

namespace xi {
namespace core {
  namespace v2 {

    template < typename traitsT >
    struct basic_mmap_stack_allocator {
      // typedef traitsT traits_type;
      enum { size = 1 << 12, max_size = 1 << 20 };

      ::boost::context::stack_context allocate() {
        // BOOST_ASSERT(traits_type::minimum_size() <= size);
        // BOOST_ASSERT(traits_type::is_unbounded() ||
        //              (traits_type::maximum_size() >= size));

        // void* limit = std::malloc(size);
        // if (!limit)
        //   throw std::bad_alloc();

        ::boost::context::stack_context ctx;
        ctx.size = max_size;
        // ctx.sp   = static_cast< char* >(limit) + ctx.size;
        ctx.sp = static_cast< char* >(mmap(nullptr,
                                           max_size,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                           -1,
                                           0)) +
                 max_size;
        if (MAP_FAILED == ctx.sp)
          throw std::bad_alloc();
#if defined(BOOST_USE_VALGRIND)
        ctx.valgrind_stack_id = VALGRIND_STACK_REGISTER(ctx.sp, limit);
#endif
        return ctx;
      }

      void deallocate(::boost::context::stack_context& ctx) {
// BOOST_ASSERT(ctx.sp);
// BOOST_ASSERT(traits_type::minimum_size() <= ctx.size);
// BOOST_ASSERT(traits_type::is_unbounded() ||
//              (traits_type::maximum_size() >= ctx.size));

#if defined(BOOST_USE_VALGRIND)
        VALGRIND_STACK_DEREGISTER(ctx.valgrind_stack_id);
#endif

        auto ptr = static_cast< char* >(ctx.sp) - max_size;
        ::munmap(ptr, max_size);
      }
    };

    typedef basic_mmap_stack_allocator<::boost::coroutines::stack_traits >
        mmap_stack_allocator;

    class generic_resumable : public resumable {
      using context_t = ext::execution_context;

      context_t _yield_ctx;
      context_t _my_ctx;
      result _result   = reschedule{};
      bool _is_running = false;

    protected:
      result resume(mut< execution_budget >) final override {
        if (!_is_running) {
          _yield_ctx  = context_t::current();
          _is_running = true;
          return *reinterpret_cast< result* >(_my_ctx());
        }
        return reschedule{};
      }

      void yield(result result) final override {
        if (_is_running) {
          _is_running = false;
          _result     = result;
          _yield_ctx(&_result);
        }
      }

      virtual void call() = 0;

      generic_resumable()
          : _yield_ctx(context_t::current())
          , _my_ctx(
                // std::allocator_arg,
                // mmap_stack_allocator{},
                [this](void*) {
                  _is_running = true;
                  call();
                  _result = done{};
                  _yield_ctx(&_result);
                }) {
        // printf("Created in %p : %p.\n", pthread_self(), this);
        // printf("%s %p\n", __PRETTY_FUNCTION__, this);
      }

      // ~generic_resumable() {
      //   printf("%s\n", __PRETTY_FUNCTION__);
      // }

      XI_CLASS_DEFAULTS(generic_resumable, no_move, no_copy);
    };
  }

  class generic_resumable : public resumable {
    using context_t = ext::execution_context;

    context_t _yield_ctx;
    context_t _my_ctx;
    resume_result _result = resume_later;
    bool _is_running      = false;

  protected:
    resume_result resume() final override {
      if (!_is_running) {
        _yield_ctx  = context_t::current();
        _is_running = true;
        return *reinterpret_cast< resume_result* >(_my_ctx());
      }
      return resume_later;
    }

    void yield(resume_result result) final override {
      if (_is_running) {
        _is_running = false;
        _result     = result;
        _yield_ctx(&_result);
      }
    }

    virtual void call() = 0;

    generic_resumable()
        : _yield_ctx(context_t::current()), _my_ctx([this](void*) {
          _is_running = true;
          call();
          _result = done;
          _yield_ctx(&_result);
        }) {
      // printf("Created in %p : %p.\n", pthread_self(), this);
      // printf("%s %p\n", __PRETTY_FUNCTION__, this);
    }

    // ~generic_resumable() {
    //   printf("%s\n", __PRETTY_FUNCTION__);
    // }

    XI_CLASS_DEFAULTS(generic_resumable, no_move, no_copy);
  };
}
}
