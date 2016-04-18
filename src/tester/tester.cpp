#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

#include "server.h"

using namespace boost::asio;
using namespace std::chrono;
using namespace boost::accumulators;

using boost::asio::ip::tcp;
using std::min;
using std::max;

class client {
  struct message {
    union {
      struct[[gnu::packed]] {
        uint8_t version;
        uint8_t type;
        uint32_t size;
        uint8_t pad[2];
        uint32_t magic;
        size_t id;
        size_t timestamp;
      };
      char buffer[256]; // = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Hello,
                        // world!";
    };
  };
  message _write;
  message _read;
  size_t _write_id  = 0;
  size_t _read_id   = 0;
  size_t _totaltime = 0;
  size_t _min_lat   = std::numeric_limits< size_t >::max();
  size_t _max_lat   = 0;
  size_t _totalsize = 0;
  size_t _bad       = 0;
  size_t _out       = 0;
  time_point< high_resolution_clock > _starttime;
  accumulator_set< size_t, stats< tag::mean > > _acc;
  tcp::socket _socket;

public:
  client(io_service &service, tcp::endpoint server) : _socket(service) {
    _write.version = 1;
    _write.type    = 1;
    _write.size    = sizeof(_write.buffer) - 8;
    _write.magic   = 'COON';
    _socket.async_connect(server, [this](auto error) { this->start(); });
  }

private:
  void start() {
    _starttime = high_resolution_clock::now();
    start_reading();
    start_writing();
  }
  void start_writing() {
    _write.id        = ++_write_id;
    _write.timestamp = duration_cast< microseconds >(
                           high_resolution_clock::now().time_since_epoch())
                           .count();
    // for (int i = 0; i < 20; ++i) {
    //     std::cout << (int)_write.buffer [i] << " ";
    // }
    // std::cout << std::endl;
    async_write(_socket,
                buffer(_write.buffer, sizeof(_write.buffer)),
                [this](auto error, auto written) {
                  ++_out;
                  if (!error && _out < 200) {
                    this->start_writing();
                  }
                });
  }
  void start_reading() {
    async_read(
        _socket,
        buffer(_read.buffer, sizeof(_read.buffer)),
        boost::asio::transfer_exactly(sizeof(_write.buffer)),
        [this](auto error, size_t read) {
          if (!error) {
            ++_read_id;
            if (_read.magic == 'COON') {
              auto timestamp = high_resolution_clock::now();
              auto ns =
                  duration_cast< microseconds >(timestamp.time_since_epoch())
                      .count();
              auto lat = ns - _read.timestamp;
              _min_lat = min(lat, _min_lat);
              _max_lat = max(lat, _max_lat);
              _totaltime += lat;
              // _acc(ns - _read.timestamp);
              if (_read_id > 0 && (_read_id % 100000) == 0) {
                auto sec =
                    duration_cast< seconds >(timestamp - _starttime).count();
                std::cout << " avg: "
                          << microseconds(_totaltime / _read_id).count() << "us"
                          // << " mean: " << mean(_acc) << "ns"
                          << " min: " << _min_lat << "ns"
                          << " max: " << _max_lat << "ns"
                          << " W.cnt: " << _write_id << " R.cnt: " << _read_id
                          << " spent: " << sec
                          << "s W.msg/s: " << (_write_id * 1.0) / sec
                          << "s R.msg/s: " << (_read_id * 1.0) / sec
                          << " bytes/s: " << (_totalsize * 1.0) / sec
                          << " bad: " << _bad << std::endl;
              }
            } else {
              ++_bad;
            }
            this->start_reading();
            if (_out == 200) {
              this->start_writing();
            }
            --_out;
          }
        });
  }
};

using namespace std;

int
main(int argc, char *argv[]) {
  io_service service[1];
  boost::thread_group g;
  for (auto i = 0; i < 1; ++i) {
    g.create_thread([ s = &service[i], argc, argv ] {
      tcp::resolver resolver(*s);
      if (string(argv[1]) == "-s") {
        new server(*s, atoi(argv[2]));
      } else {
        tcp::resolver::query query(argv[1], argv[2]);
        resolver.async_resolve(query, [s](auto error, auto iterator) {
          for (auto i = 0; i < 100; ++i) {
            new client(*s, *iterator);
          }
        });
      }
      s->run();
    });
  }

  g.join_all();

  return 0;
}
