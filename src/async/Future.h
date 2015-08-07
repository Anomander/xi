#pragma once

#include "ext/Configure.h"
#include "ext/OverloadRank.h"
#include "async/Engine.h"
#include "util/SpinLock.h"

namespace xi {
namespace async {
  template < class... >
  class Future;
  template < class... >
  class Promise;

  template < class... Args >
  Future< Args... > makeReadyFuture(Args&&... args);
  template < class... Args >
  Future< Args... > makeReadyFuture(exception_ptr);
  Future<> makeReadyFuture(exception_ptr);

  struct BrokenPromiseException : public std::logic_error {
    BrokenPromiseException() : std::logic_error("This promise has been broken") {}
  };

  enum LaunchPolicy { kAsync, kInline };

  namespace detail {

    template < class... >
    class State;

    template < class T >
    struct FutureFromCallable {
      using Result = Future< T >;

      template < class Func, class... Args >
      static Future< T > apply(Func&& f, tuple< Args... >&& args) {
        static_assert(is_same< result_of_t< Func(Args && ...) >, T >::value, "Return type of Func must be T.");
        try {
          return makeReadyFuture< T >(Unpack::apply(forward< Func >(f), forward< tuple< Args... > >(args)));
        } catch (...) {
          return makeReadyFuture< T >(current_exception());
        }
      }
    };

    template < class T >
    struct CallableApplier {
      template < class Func, class... Args >
      static void apply(Func&&, State< Args... >&, Promise< T >&);

      template < class Func, class... Args >
      static void applyValue(Func&& f, tuple< Args... >&& value, Promise< T >& promise);
    };

    template <>
    struct CallableApplier< void > {
      template < class Func, class... Args >
      static void apply(Func&&, State< Args... >&, Promise<>&);

      template < class Func, class... Args >
      static void applyValue(Func&& f, tuple< Args... >&& value, Promise<>& promise);
    };

    template < class Func, class... Ts >
    using FutureFromCallableResult = typename FutureFromCallable< result_of_t< Func(Ts&&...) > >::Result;

    template < class Func, class... Ts >
    using FutureFromCallablePromise = typename FutureFromCallableResult< Func, Ts... >::PromiseType;

    template < class Func, class... Args >
    decltype(auto) makeFutureFromCallable(Func&& f, Args&&... args) {
      return detail::FutureFromCallable< result_of_t< Func(Args && ...) > >::apply(forward< Func >(f),
                                                                                   make_tuple(args...));
    }

    template < class Func, class... Args >
    decltype(auto) makeFutureFromCallable(Func&& f, tuple< Args... >&& args) {
      return detail::FutureFromCallable< result_of_t< Func(Args && ...) > >::apply(forward< Func >(f),
                                                                                   forward< tuple< Args... > >(args));
    }

    template < class... Ts >
    class State {
    protected:
      struct Unfulfilled {};

      variant< Unfulfilled, tuple< Ts... >, exception_ptr > _value = Unfulfilled{};

      struct ValueVisitor : public static_visitor< tuple< Ts... > > {
        tuple< Ts... > operator()(tuple< Ts... >& value) const { return move(value); }
        [[noreturn]] tuple< Ts... > operator()(exception_ptr& ex) const { rethrow_exception(move(ex)); }
        [[noreturn]] tuple< Ts... > operator()(Unfulfilled& ex) const { std::terminate(); }
      };

    protected:
      auto& value() noexcept { return _value; }

    public:
      template < class... Args >
      State(Args&&... args)
          : _value(forward< Args >(args)...) {}
      State(State&& other) : _value(move(other._value)) { other._value = Unfulfilled{}; }
      State& operator=(State&& other) {
        if (this != &other) {
          this->~State();
          new (this) State(move(other));
        }
        return *this;
      }
      tuple< Ts... > get() {
        assert(this->isReady());
        return apply_visitor(ValueVisitor{}, _value);
      }
      exception_ptr getException() {
        assert(isException());
        return ext::get< exception_ptr >(_value);
      }
      bool isReady() const { return (nullptr == ext::get< Unfulfilled >(&_value)); }
      bool isException() const { return (nullptr != ext::get< exception_ptr >(&_value)); }
    };

    template < class... Ts >
    class SharedState : public virtual ownership::StdShared, public State< Ts... > {
      enum { kInlineCallbackStorage = 64 };
      char _callbackStorage[kInlineCallbackStorage];

      function< void(SharedState*)> _callback;
      SpinLock _spinLock;

    private:
      template < LaunchPolicy Policy, class Func >
      struct OwningCallback {
        OwningCallback(Func&& f) : _f(make_unique< Func >(move(f))){};

        void operator()(SharedState* state) {
          if (kInline == Policy) {
            _run(state);
          } else {
            async2::schedule([ state = share(state), this /*held by state*/ ]() mutable { _run(addressOf(state)); });
          }
        }

      private:
        void _run(SharedState* state) {
          XI_SCOPE(exit) { _f.reset(); };
          (*_f)(*state);
        }
        unique_ptr< Func > _f;
      };

      template < LaunchPolicy Policy, class Func >
      struct RefCallback {
        RefCallback(Func&& f) : _f(move(f)){};

        void operator()(SharedState* state) {
          if (kInline == Policy) {
            _run(state);
          } else {
            async2::schedule([ state = share(state), this /*held by state*/ ]() mutable { _run(addressOf(state)); });
          }
        }

      private:
        void _run(SharedState* state) {
          XI_SCOPE(exit) { _f.~Func(); };
          _f(*state);
        }
        Func _f;
      };

      void maybeCallback() {
        if (_callback) {
          _callback(this);
        }
      }

    public:
      using State< Ts... >::State;

      template < class... Args >
      void set(tuple< Args... >&& value) {
        auto lock = makeLock(_spinLock);
        this->value() = move(value);
        maybeCallback();
      }

      template < class... Args >
      void set(Args&&... value) {
        set(make_tuple(forward< Args >(value)...));
      }

      void set(exception_ptr&& ptr) {
        auto lock = makeLock(_spinLock);
        this->value() = move(ptr);
        maybeCallback();
      }

      template < LaunchPolicy Policy, class Func >
      detail::FutureFromCallableResult< Func, Ts... > setContinuation(Func&& f) {
        auto lock = makeLock(_spinLock);
        using ResultType = result_of_t< Func(Ts && ...) >;

        if (this->isReady()) {
          using ResultType = result_of_t< Func(Ts && ...) >;
          if (this->isException()) {
            return makeReadyFuture< ResultType >(this->getException());
          }
          if (kInline == Policy) {
            return detail::makeFutureFromCallable(forward< Func >(f), this->get());
          } else {
            detail::FutureFromCallablePromise< Func, Ts... > promise;
            auto future = promise.getFuture();
            async2::schedule([ value = this->get(), promise = move(promise), f = forward< Func >(f) ]() mutable {
              CallableApplier< ResultType >::applyValue(move(f), move(value), promise);
            });
            return future;
          }
        }

        detail::FutureFromCallablePromise< Func, Ts... > promise;
        auto future = promise.getFuture();
        auto callback = [ promise = move(promise), f = forward< Func >(f) ](State< Ts... > & state) mutable {
          CallableApplier< ResultType >::apply(move(f), state, promise);
        };

        if (kInlineCallbackStorage >= sizeof(RefCallback< Policy, decltype(callback) >)) {
          auto cb = new (_callbackStorage) RefCallback< Policy, decltype(callback) >{move(callback)};
          _callback = reference(*cb);
        } else {
          auto cb = new (_callbackStorage) OwningCallback< Policy, decltype(callback) >{move(callback)};
          _callback = reference(*cb);
        }
        return future;
      }
    };
    static constexpr struct ReadyMadeTag { } ReadyMade{}; }

  template < class... Ts >
  class Future {
  public:
    using PromiseType = Promise< Ts... >;
    using StateType = detail::State< Ts... >;
    using SharedStateType = detail::SharedState< Ts... >;

    Future(Future&& other) : _state(move(other._state)) {}
    Future& operator=(Future&& other) {
      if (this != &other) {
        this->~Future();
        new (this) Future(move(other));
      }
      return *this;
    }

    bool isReady() const { return state()->isReady(); }

    bool isException() const { return state()->isException(); }

    template < class Func >
    detail::FutureFromCallableResult< Func, Ts... > then(Func&& f) {
      return then(kInline, forward< Func >(f));
    }

    template < class Func >
    detail::FutureFromCallableResult< Func, Ts... > then(LaunchPolicy policy, Func&& f) {
      using ResultType = result_of_t< Func(Ts && ...) >;

      if (isLocal()) {
        assert(isReady()); // local state must always be ready
        if (isException()) {
          return makeReadyFuture< ResultType >(state()->getException());
        }
        if (kInline == policy) {
          return detail::makeFutureFromCallable(forward< Func >(f), state()->get());
        } else {
          detail::FutureFromCallablePromise< Func, Ts... > promise;
          auto future = promise.getFuture();
          async2::schedule([ value = state()->get(), promise = move(promise), f = forward< Func >(f) ]() mutable {
            detail::CallableApplier< ResultType >::applyValue(forward< Func >(f), move(value), promise);
          });
          return future;
        }
      }
      assert(!isLocal()); // must be remote
      switch (policy) {
        case kInline:
          return ext::get< own< SharedStateType > >(_state)->template setContinuation< kInline >(forward< Func >(f));
        case kAsync:
          return ext::get< own< SharedStateType > >(_state)->template setContinuation< kAsync >(forward< Func >(f));
        default:
          assert(false); // invalid launch policy
      }
    }

  protected:
    template < class... Args >
    Future(detail::ReadyMadeTag, Args&&... args)
        : Future(SelectRank, forward< Args >(args)...) {}

    template < class... Args >
    Future(Rank< 1 >, tuple< Args... >&& args)
        : _state(move(args)) {}

    Future(Rank< 1 >, exception_ptr&& ex) : _state(move(ex)) {}

    template < class... Args >
    Future(Rank< 2 >, Args&&... args)
        : _state(make_tuple(forward< Args >(args)...)) {}

    explicit Future(own< SharedStateType > state) : _state(move(state)) {}

    template < class... Args >
    friend Future< Args... > makeReadyFuture(Args&&... args);
    template < class... Args >
    friend Future< Args... > makeReadyFuture(exception_ptr);
    friend Future<> makeReadyFuture(exception_ptr);

    bool isLocal() const { return !!ext::get< StateType >(&_state); }

  private:
    StateType const* state() const { return const_cast< Future* >(this)->state(); }

    StateType* state() {
      auto* local = ext::get< StateType >(&_state);
      if (local) {
        return local;
      }
      return addressOf(ext::get< own< SharedStateType > >(_state));
    }

  private:
    template < class... >
    friend class Promise;
    variant< StateType, own< SharedStateType > > _state;
  };

  template <>
  struct Future< void > : public Future<> {
    using Future<>::PromiseType;
    using Future<>::Future;
  };

  template < class... Args >
  Future< Args... > makeReadyFuture(Args&&... args) {
    return Future< Args... >(detail::ReadyMade, forward< Args >(args)...);
  }
  template < class... Args >
  Future< Args... > makeReadyFuture(exception_ptr ex) {
    return Future< Args... >(detail::ReadyMade, move(ex));
  }
  Future<> makeReadyFuture(exception_ptr ex) { return Future<>(detail::ReadyMade, move(ex)); }

  static_assert(is_same< Future< int >, decltype(makeReadyFuture< int >(exception_ptr{})) >::value,
                "makeReadyFuture with exception must create correctly-typed future when specified");
  static_assert(is_same< Future<>, decltype(makeReadyFuture(exception_ptr{})) >::value,
                "makeReadyFuture with exception must create empty-typed future unless specified");

  template < class... Ts >
  class Promise {
  public:
    using FutureType = Future< Ts... >;
    using StateType = detail::SharedState< Ts... >;

    Promise() = default;
    Promise(Promise&& other) = default;
    Promise& operator=(Promise&& other) = default;

    ~Promise() noexcept {
      if (_state /* might've been moved out */ && !_state->isReady()) {
        _state->set(make_exception_ptr(BrokenPromiseException{}));
      }
    }

    FutureType getFuture() { return FutureType(_state); }

    template < class... Args >
    void setValue(Args&&... args) {
      _state->set(forward< Args >(args)...);
    }

    void setException(exception_ptr ex) { _state->set(move(ex)); }

  private:
    own< StateType > _state = make< StateType >();
  };

  namespace detail {

    template < class T >
    template < class Func, class... Args >
    void CallableApplier< T >::apply(Func&& f, State< Args... >& state, Promise< T >& promise) {
      static_assert(is_same< result_of_t< Func(Args && ...) >, T >::value, "Return type of Func must be T.");
      assert(state.isReady());
      if (state.isException()) {
        promise.setException(state.getException());
      } else {
        applyValue(forward< Func >(f), state.get(), promise);
      }
    }

    template < class T >
    template < class Func, class... Args >
    void CallableApplier< T >::applyValue(Func&& f, tuple< Args... >&& value, Promise< T >& promise) {
      static_assert(is_same< result_of_t< Func(Args && ...) >, T >::value, "Return type of Func must be T.");
      try {
        promise.setValue(Unpack::apply(forward< Func >(f), move(value)));
      } catch (...) {
        promise.setException(current_exception());
      }
    }

    template < class Func, class... Args >
    void CallableApplier< void >::apply(Func&& f, State< Args... >& state, Promise<>& promise) {
      static_assert(is_same< result_of_t< Func(Args && ...) >, void >::value, "Return type of Func must be void.");
      assert(state.isReady());
      if (state.isException()) {
        promise.setException(state.getException());
      } else {
        applyValue(forward< Func >(f), state.get(), promise);
      }
    }

    template < class Func, class... Args >
    void CallableApplier< void >::applyValue(Func&& f, tuple< Args... >&& value, Promise<>& promise) {
      static_assert(is_same< result_of_t< Func(Args && ...) >, void >::value, "Return type of Func must be void.");
      try {
        Unpack::apply(forward< Func >(f), move(value));
        promise.setValue();
      } catch (...) {
        promise.setException(current_exception());
      }
    }

    template <>
    struct FutureFromCallable< void > {
      using Result = Future<>;
      template < class Func, class... Args >
      static Future<> apply(Func&& f, tuple< Args... >&& args) {
        static_assert(is_same< result_of_t< Func(Args && ...) >, void >::value, "Return type of Func must be void.");
        try {
          Unpack::apply(forward< Func >(f), forward< tuple< Args... > >(args));
          return makeReadyFuture();
        } catch (...) {
          return makeReadyFuture(current_exception());
        }
      }
    };

    template < class... Ts >
    struct FutureFromCallable< Future< Ts... > > {
      using Result = Future< Ts... >;
      template < class Func, class... Args >
      static Future< Ts... > apply(Func&& f, tuple< Args... >&& args) {
        static_assert(is_same< result_of_t< Func(Args && ...) >, Future< Ts... > >::value,
                      "Return type of Func must be Future <Ts...>.");
        try {
          return Unpack::apply(forward< Func >(f), forward< tuple< Args... > >(args));
        } catch (...) {
          return makeReadyFuture< Ts... >(current_exception());
        }
      }
    };
  }
}
}
