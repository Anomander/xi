#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/lockfree.h"
#include "xi/core/detail/intrusive.h"
#include "xi/core/runtime.h"
#include "xi/core/resumable.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace core {

  template < class T >
  struct abstract_message_bus : virtual ownership::std_shared {
    XI_CLASS_DEFAULTS(abstract_message_bus, virtual_dtor);
    virtual void put(T&& value) = 0;
    virtual void get(T* value)  = 0;
  };

  template < class T, usize Size >
  struct buffer {
    array< T, Size > _queue;
    atomic< u64 > _head{0};
    atomic< u64 > _tail{0};

  public:
    bool push(T&& value) {
      auto tail = _tail.load(memory_order_relaxed);
      for (;;) {
        auto head      = _head.load(memory_order_acquire);
        auto next_tail = _next_idx(tail);
        if (next_tail == head) {
          return false; // last I checked there was no space in the queue
        }
        if (_tail.compare_exchange_strong(
                tail, next_tail, memory_order_relaxed, memory_order_release)) {
          _push_at(next_tail, forward< T >(value));
          return true;
        }
      }
    }

    bool pop(T* value) {
      auto head = _head.load(memory_order_relaxed);
      for (;;) {
        auto tail = _tail.load(memory_order_acquire);
        if (tail == head) {
          return false; // last I checked there were no items in the queue
        }
        auto next_head = _next_idx(head);
        if (_head.compare_exchange_strong(
                head, next_head, memory_order_relaxed, memory_order_release)) {
          _pop_at(head, value);
          return true;
        }
      }
    }

  private:
    void _push_at(u64 idx, T&& value) {
      new (&_queue[idx]) T(forward< T >(value));
    }

    void _pop_at(u64 idx, T* value) {
      new (value) T(move(_queue[idx]));
    }

    u64 _next_idx(u64 cur) {
      return (cur + 1) % Size;
    }
  };

  template < class T >
  struct buffer< T, 0 > {
    bool push(T&&) {
      return false;
    }

    bool pop(T*) {
      return false;
    }
  };

  template < class T, usize Size >
  class message_bus final : public abstract_message_bus< T > {

    struct receiver {
      T* storage;
      resumable* process;
    };

    spin_lock _lock;
    deque< resumable* > _blocked_senders;
    deque< receiver > _blocked_receivers;

    buffer< T, Size > _buffer;

  public:
    void put(T&& value) final override {
      for (;;) {
        auto lock = make_unique_lock(_lock);
        if (maybe_deliver(forward< T >(value))) {
          return; // delivered directly
        } else if (_buffer.push(forward< T >(value))) {
          return; // waiting for receiver
        }
        // neither receivers, nor space in buffer are available,
        // must block until it frees up
        auto self = runtime.local_worker().current_resumable();
        _blocked_senders.push_back(self);
        lock.unlock();
        self->block();
      }
    }

    void get(T* value) final override {
      auto lock = make_unique_lock(_lock);
      if (_buffer.pop(value)) {
        maybe_fill();
        return; // received from buffer
      }
      auto self = runtime.local_worker().current_resumable();
      _blocked_receivers.push_back({value, self});
      maybe_fill();
      lock.unlock();
      self->block();
    }

  private:
    bool maybe_deliver(T&& value) {
      if (!_blocked_receivers.empty()) {
        XI_SCOPE(exit) {
          _blocked_receivers.pop_front();
        };
        auto& recv = _blocked_receivers.front();
        if (_buffer.pop(recv.storage)) {
          return false; // delivered from buffer, still need to write my data
        }
        new (recv.storage) T(forward< T >(value));
        recv.process->unblock();
        return true;
      }
      return false;
    }

    void maybe_fill() {
      if (!_blocked_senders.empty()) {
        XI_SCOPE(exit) {
          _blocked_senders.pop_front();
        };
        _blocked_senders.front()->unblock();
      }
    }
  };

  template < class T >
  class channel_base {
    own< abstract_message_bus< T > > _bus;

  protected:
    channel_base(own< abstract_message_bus< T > > bus) : _bus(move(bus)) {
    }

    auto& bus() {
      return _bus;
    }
  };

  template < class T >
  class ichannel : protected channel_base< T > {
  public:
    ichannel(own< abstract_message_bus< T > > bus)
        : channel_base< T >(move(bus)) {
    }

    void recv(T& value) {
      this->bus()->get(&value);
    }

    T recv() {
      T value;
      recv(value);
      return value;
    }
  };

  template < typename T >
  struct channel_iterator
      : std::iterator< std::output_iterator_tag, T > {
    using super_t = std::iterator< std::output_iterator_tag, T >;

    T operator*() {
      return in.recv();
    }

    // prefix
    channel_iterator< T >& operator++() {
      return *this;
    }

    bool operator==(const channel_iterator< T >& ) const {
      return false; // FIXME
    }

    bool operator!=(const channel_iterator< T >& rhs) const {
      return ! operator==(rhs);
    }

  private:
    ichannel<T> in;
    channel_iterator< T >(ichannel<T> i) : in(move(i)) {
    }
    template<class> friend class channel;
  };

  template < class T >
  class ochannel : protected channel_base< T > {
  protected:
    ochannel(own< abstract_message_bus< T > > bus)
        : channel_base< T >(move(bus)) {
    }

  public:
    void send(T value) {
      this->bus()->put(move(value));
    }

    channel_iterator<T> begin() { return {*this}; }
    channel_iterator<T> end() { return {*this}; }
  };

  template < class T >
  class channel : protected channel_base< T > {
  public:
    channel(own< abstract_message_bus< T > > bus)
        : channel_base< T >(move(bus)) {
    }

    operator ichannel< T >() {
      return ichannel< T >(this->bus());
    }

    operator ochannel< T >() {
      return ochannel< T >(this->bus());
    }

    void recv(T& value) {
      this->bus()->get(&value);
    }

    T recv() {
      T value;
      recv(value);
      return value;
    }

    void send(T value) {
      this->bus()->put(move(value));
    }

    channel_iterator<T> begin() { return {*this}; }
    channel_iterator<T> end() { return {*this}; }
  };

  template < class T, usize BufferSize = 0 >
  auto make_channel() {
    return channel< T >(make< message_bus< T, BufferSize > >());
  }
}
}
