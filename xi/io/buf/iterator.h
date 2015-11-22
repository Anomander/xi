#pragma once

#include "xi/io/buf/chain.h"
#include "xi/ext/configure.h"

namespace xi {
namespace io {

  class buffer::chain::iterator
      : public boost::iterator_facade< iterator, uint8_t,
                                       boost::random_access_traversal_tag > {
    mut< buffer > _head;
    mut< buffer > _curr;
    uint8_t* _pos;

  public:
    iterator(iterator const&) = default;
    iterator& operator=(iterator const&) = default;
    iterator(iterator&&) = default;
    iterator& operator=(iterator&&) = default;

  private:
    friend class chain;

    iterator(mut< buffer >, mut< buffer >, size_t);

  private:
    friend class boost::iterator_core_access;

    reference dereference() const;
    bool equal(iterator const&) const;
    void increment();
    void decrement();
    void advance(int);
    int distance_to(iterator const&) const;

  private:
    size_t left_in_curr() const;
    size_t offset_in_curr() const;
    uint8_t* end_of_curr() const;
    uint8_t* pointer() const;
    mut< buffer > buf() const;
  };

  inline buffer::chain::iterator::iterator(mut< buffer > head,
                                           mut< buffer > curr, size_t offset)
      : _head(head), _curr(curr), _pos(_curr->begin() + offset) {}

  inline auto buffer::chain::iterator::dereference() const -> reference {
    return *_pos;
  }
  inline size_t buffer::chain::iterator::left_in_curr() const {
    return _curr->end() - _pos;
  }

  inline size_t buffer::chain::iterator::offset_in_curr() const {
    return _pos - _curr->begin();
  }

  inline uint8_t* buffer::chain::iterator::end_of_curr() const {
    return _curr->end();
  }
  inline uint8_t* buffer::chain::iterator::pointer() const { return _pos; }
  inline mut< buffer > buffer::chain::iterator::buf() const { return _curr; }
}
}
