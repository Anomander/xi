#pragma once

namespace xi {
namespace io {

  template < class ValueType, int Level, int Name,
             int Length = sizeof(ValueType) >
  struct socket_option {
    socket_option(ValueType value) : _value(value) {}

    constexpr int name() { return Name; }
    constexpr int level() { return Level; }
    constexpr int length() { return Length; }
    ValueType value() { return _value; }

  private:
    ValueType _value;
  };

  template < int level, int name >
  struct boolean_socket_option : socket_option< int, level, name > {
    using base = socket_option< int, level, name >;

    using base::socket_option;

    boolean_socket_option(bool value) : base(value ? 1 : 0) {}

    operator bool() { return (bool)base::value(); }

    static boolean_socket_option yes;
    static boolean_socket_option no;
  };

  template < int level, int name >
  boolean_socket_option< level, name >
      boolean_socket_option< level, name >::yes(true);
  template < int level, int name >
  boolean_socket_option< level, name > boolean_socket_option< level, name >::no(
      false);
}
}
