#pragma once

#include "xi/ext/preprocessor.h"

#define XI_DEFAULT_COPIABLE(Class)                                             \
  Class(Class const &) = default;                                              \
  Class &operator=(Class const &) = default;

#define XI_DEFAULT_MOVABLE(Class)                                              \
  Class(Class &&) = default;                                                   \
  Class &operator=(Class &&) = default;

#define XI_NOT_COPIABLE(Class)                                                 \
  Class(Class const &) = delete;                                               \
  Class &operator=(Class const &) = delete;

#define XI_NOT_MOVABLE(Class)                                                  \
  Class(Class &&) = delete;                                                    \
  Class &operator=(Class &&) = delete;

#define XI_VIRTUAL_DTOR(Class) virtual ~Class() = default;

#define XI_CTOR(Class) Class() = default;

#define XI_DEFAULT_COPIABLE_AND_MOVABLE(Class)                                 \
  XI_DEFAULT_COPIABLE(Class)                                                   \
  XI_DEFAULT_MOVABLE(Class)

#define XI_ONLY_MOVABLE(Class)                                                 \
  XI_DEFAULT_MOVABLE(Class)                                                    \
  XI_NOT_COPIABLE(Class)

#define XI_ONLY_COPIABLE(Class)                                                \
  XI_DEFAULT_COPIABLE(Class)                                                   \
  XI_NOT_MOVABLE(Class)

#define XI_NEITHER_MOVABLE_NOR_COPIABLE(Class)                                 \
  XI_NOT_COPIABLE(Class)                                                       \
  XI_NOT_MOVABLE(Class)

#define __XI_CLASS_SEMANTIC(r, Class, elem)                                    \
  BOOST_PP_CAT(__XI_CLASS_SEMANTIC_, elem)(Class)

#define __XI_CLASS_SEMANTIC_no_move(Class) XI_NOT_MOVABLE(Class)
#define __XI_CLASS_SEMANTIC_no_copy(Class) XI_NOT_COPIABLE(Class)

#define __XI_CLASS_SEMANTIC_move(Class) XI_DEFAULT_MOVABLE(Class)
#define __XI_CLASS_SEMANTIC_copy(Class) XI_DEFAULT_COPIABLE(Class)

#define __XI_CLASS_SEMANTIC_virtual_dtor(Class) XI_VIRTUAL_DTOR(Class)

#define __XI_CLASS_SEMANTIC_ctor(Class) XI_CTOR(Class)

#define XI_CLASS_DEFAULTS(Class, ...)                                          \
  BOOST_PP_SEQ_FOR_EACH(                                                       \
      __XI_CLASS_SEMANTIC, Class, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__));
