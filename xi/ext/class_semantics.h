#pragma once

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
