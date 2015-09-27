#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/Modifiers.h"
#include "xi/io/pipeline2/detail/Links.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

    template < class M0 >
    struct PipeLink {
    private:
      mut< ReadLink< M0 > > _head = nullptr;
      mut< WriteLink< M0 > > _tail = nullptr;

    protected:
      void maybeUpdateHead(mut< GenericFilterContext > ctx) {
        auto readM0 = dynamic_cast< mut< ReadLink< M0 > > >(ctx);
        if (readM0) {
          _head = readM0;
        }
        if (!_tail) {
          auto writeM0 = dynamic_cast< mut< WriteLink< M0 > > >(ctx);
          if (writeM0) {
            _tail = writeM0;
          }
        }
      }

      void maybeUpdateTail(mut< GenericFilterContext > ctx) {
        auto writeM0 = dynamic_cast< mut< WriteLink< M0 > > >(ctx);
        if (writeM0) {
          _tail = writeM0;
        }
        if (!_head) {
          auto readM0 = dynamic_cast< mut< ReadLink< M0 > > >(ctx);
          if (readM0) {
            _head = readM0;
          }
        }
      }

    public:
      void read(M0 m) { _head->read(move(m)); }
      void write(M0 m) { _tail->write(move(m)); }
    };

    template < class M0 >
    struct PipeLink< ReadOnly< M0 > > {
    private:
      mut< ReadLink< M0 > > _head;

    protected:
      void maybeUpdateHead(mut< GenericFilterContext > ctx) {
        auto readM0 = dynamic_cast< mut< ReadLink< M0 > > >(ctx);
        if (readM0) {
          _head = readM0;
        }
      }
      void maybeUpdateTail(mut< GenericFilterContext > ctx) {
        if (!_head) {
          auto readM0 = dynamic_cast< mut< ReadLink< M0 > > >(ctx);
          if (readM0) {
            _head = readM0;
          }
        }
      }

    public:
      void read(M0 m) { _head->read(move(m)); }
      void write(M0 m) = delete;
    };

    template < class M0 >
    struct PipeLink< WriteOnly< M0 > > {
    private:
      mut< WriteLink< M0 > > _tail;

    protected:
      void maybeUpdateTail(mut< GenericFilterContext > ctx) {
        auto writeM0 = dynamic_cast< mut< WriteLink< M0 > > >(ctx);
        if (writeM0) {
          _tail = writeM0;
        }
      }
      void maybeUpdateHead(mut< GenericFilterContext > ctx) {
        if (!_tail) {
          auto writeM0 = dynamic_cast< mut< WriteLink< M0 > > >(ctx);
          if (writeM0) {
            _tail = writeM0;
          }
        }
      }

    public:
      void write(M0 m) { _tail->write(move(m)); }
      void read(M0 m) = delete;
    };

  }
}
}
}
