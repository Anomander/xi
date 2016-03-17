#pragma once

namespace xi {
namespace io {
  namespace net {

    enum socket_option_access { READ_ONLY, READ_WRITE };

    template < class ValueType, int Level, int Name,
               socket_option_access Access = READ_WRITE,
               int Length = sizeof(ValueType) >
    class socket_option {
      ValueType _value;

    public:
      enum : int {
        level = Level,
        name = Name,
        length = Length,
      };
      using value_t = ValueType;

      socket_option(ValueType value) : _value(value) {
      }

      operator ValueType() {
        return _value;
      }

      ValueType value() {
        return _value;
      }

      static socket_option val(ValueType v) {
        return {v};
      }
    };

    template < int level, int name, socket_option_access Access = READ_WRITE >
    using numeric_socket_option = socket_option< int, level, name, Access >;

    template < int level, int name, socket_option_access Access = READ_WRITE >
    struct boolean_socket_option : socket_option< int, level, name, Access > {
      using base = socket_option< int, level, name >;

      using base::base;

      operator bool() {
        return (bool)base::value();
      }

      static boolean_socket_option yes;
      static boolean_socket_option no;
    };

    template < int L, int N, socket_option_access A >
    boolean_socket_option< L, N, A > boolean_socket_option< L, N, A >::yes(1);

    template < int L, int N, socket_option_access A >
    boolean_socket_option< L, N, A > boolean_socket_option< L, N, A >::no(0);
  }
}
}
