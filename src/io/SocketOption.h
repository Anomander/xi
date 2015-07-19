#pragma once

namespace xi {
namespace io {

template < class ValueType, int Level, int Name, int Length = sizeof(ValueType) > struct SocketOption {
  SocketOption(ValueType value) : _value(value) {}

  constexpr int name() { return Name; }
  constexpr int level() { return Level; }
  constexpr int length() { return Length; }
  ValueType value() { return _value; }

private:
  ValueType _value;
};

template < int Level, int Name > struct BooleanSocketOption : SocketOption< int, Level, Name > {
  using Base = SocketOption< int, Level, Name >;

  using Base::SocketOption;

  BooleanSocketOption(bool value) : Base(value ? 1 : 0) {}

  operator bool() { return (bool)Base::value(); }

  static BooleanSocketOption yes;
  static BooleanSocketOption no;
};

template < int Level, int Name > BooleanSocketOption< Level, Name > BooleanSocketOption< Level, Name >::yes(true);
template < int Level, int Name > BooleanSocketOption< Level, Name > BooleanSocketOption< Level, Name >::no(false);
}
}
