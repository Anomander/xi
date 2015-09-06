#pragma once

#include "ext/Configure.h"
#include "io/DataMessage.h"

namespace xi {
namespace io {

  class StreamBuffer {
    enum { kPageSize = 4096, kChunkSize = 8 * kPageSize };
    struct Chunk {
      uint8_t* data = new uint8_t[kChunkSize];
      uint8_t* wCursor = data;
      uint8_t* rCursor = data;

      ~Chunk() { delete[] data; }
      uint8_t * begin() const noexcept { return data; }
      uint8_t * end() const noexcept { return data + kChunkSize; }
      size_t size() const { return wCursor - rCursor; }
      size_t capacity() const { return end() - wCursor; }

      void consume(size_t sz) {
        auto actual = min(size(), sz);
        rCursor += actual;
      }
      void push(mut<ByteRange> range) {
        auto actual = min(capacity(), range->size);
        ::memcpy(wCursor, range->data, actual);
        wCursor += actual;
        range->consume(actual);
      }
    };
    size_t _totalSize = 0UL;
    size_t _freeCapacity = 0UL;
    deque< Chunk > _chunks;
    deque< Chunk >::iterator _writeCursor = _chunks.end();

  public:
    /// Exception safety: strong; either the entire range is pushed or nothing is pushed
    void push(ByteRange range) {
      if (range.empty()) {
        return;
      }
      allocateCapacity(range.size);
      copyNoAllocate(range);
    }

    bool empty() const { return _totalSize == 0; }
    size_t size() const { return _totalSize; }

    size_t fillVec(ByteRange* vec, size_t max) const {
      if (empty()) {
        return 0;
      }
      auto it = begin(_chunks);
      size_t i = 0;
      for (; i < max && it != end(_chunks); ++i, ++it) {
        if (it->size() == 0) {
          break;
        }
        vec[i].data = it->rCursor;
        vec[i].size = it->size();
      }
      return i;
    }

    void consume(size_t size) {
      auto it = begin(_chunks);
      while (it->size() <= size && it != _writeCursor) {
        size -= it->size();
        ++it;
      }
      _chunks.erase(begin(_chunks), it);
      it->consume(size);
      _totalSize -= min(size, _totalSize);
    }

  private:
    void allocateCapacity(size_t needed) {
      if (_writeCursor == end(_chunks)) {
        _chunks.emplace_back();
        _writeCursor = prev(end(_chunks));
        _freeCapacity += _writeCursor->capacity();
      }
      while (_freeCapacity < needed) {
        _chunks.emplace_back();
        _freeCapacity += _chunks.back().capacity();
      }
    }

    void copyNoAllocate(ByteRange range) {
      _freeCapacity -= range.size;
      do{
        _totalSize += range.size;
        _writeCursor->push(edit(range));
      } while (! range.empty() && ++_writeCursor != end(_chunks));
    }
  };
}
}
