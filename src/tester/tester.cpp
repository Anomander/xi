#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>

#include "tester/server.h"

using namespace boost::asio;
using namespace std::chrono;
using namespace boost::accumulators;

using boost::asio::ip::tcp;
using std::min;
using std::max;


class client
{
    struct message {
      union {
        struct [[gnu::packed]]{
          uint8_t version;
          uint8_t type;
          uint32_t size;
          uint8_t pad[2];
          uint32_t magic;
          size_t id;
          size_t timestamp;
        };
        char buffer [256];// = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Hello, world!";
      };
    };
    message _write;
    message _read;
    size_t _writeId = 0;
    size_t _readId = 0;
    size_t _totaltime = 0;
    size_t _minLat = std::numeric_limits<size_t>::max();
    size_t _maxLat = 0;
    size_t _totalsize = 0;
    size_t _bad = 0;
  size_t _out = 0;
    time_point <high_resolution_clock> _starttime;
    accumulator_set<size_t, stats <tag::mean>> _acc;
    tcp::socket _socket;

public:
    client(io_service& service, tcp::endpoint server)
        : _socket(service)
    {
      _write.version = 1;
      _write.type = 1;
      _write.size = sizeof(_write.buffer) - 8;
      _write.magic = 'COON';
        _socket.async_connect(server, [this] (auto error) {
            this->start();
        });
    }

private:
    void start() {
        _starttime = high_resolution_clock::now();
        startReading();
        startWriting();
    }
    void startWriting() {
        _write.id = ++ _writeId;
        _write.timestamp = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
        // for (int i = 0; i < 20; ++i) {
        //     std::cout << (int)_write.buffer [i] << " ";
        // }
        // std::cout << std::endl;
        async_write(_socket, buffer (_write.buffer, sizeof (_write.buffer)), [this] (auto error, auto written) {
            ++ _out;
            if (! error && _out < 200) {
                this->startWriting();
            }
        });
    }
    void startReading() {
        async_read(
            _socket
          , buffer (_read.buffer, sizeof (_read.buffer))
          , boost::asio::transfer_exactly(sizeof(_write.buffer))
          , [this] (auto error, size_t read) {
                if (! error) {
                    ++_readId;
                    if (_read.magic == 'COON') {
                        auto timestamp = high_resolution_clock::now();
                        auto ns = duration_cast<microseconds>(timestamp.time_since_epoch()).count();
                        auto lat = ns - _read.timestamp;
                        _minLat = min (lat, _minLat);
                        _maxLat = max (lat, _maxLat);
                        _totaltime += lat;
                        // _acc(ns - _read.timestamp);
                        if (_readId > 0 && (_readId % 100000) == 0) {
                            auto sec = duration_cast<seconds>(timestamp - _starttime).count();
                            std::cout << " avg: " << microseconds(_totaltime /_readId).count() << "us"
                                      // << " mean: " << mean(_acc) << "ns"
                                      << " min: " << _minLat << "ns"
                                      << " max: " << _maxLat << "ns"
                                      << " W.cnt: " << _writeId
                                      << " R.cnt: " << _readId
                                      << " spent: " << sec
                                      << "s W.msg/s: " << (_writeId * 1.0) / sec
                                      << "s R.msg/s: " << (_readId * 1.0) / sec
                                      << " bytes/s: " << (_totalsize * 1.0) / sec
                                      << " bad: " << _bad
                                      << std::endl;
                        }
                    } else {
                        ++_bad;
                    }
                    this->startReading();
                    if (_out == 200) {
                      this->startWriting();
                    }
                    --_out;
                }
            }
        );
    }
};

using namespace std;

int main(int argc, char* argv[]) {

    io_service service;
    tcp::resolver resolver (service);
    if (string(argv[1]) ==  "-s") {
        new server(service, atoi(argv[2]));
    } else {
        tcp::resolver::query query(argv[1], argv[2]);
        resolver.async_resolve(query, [& service] (auto error, auto iterator) {
            new client (service, *iterator);
        });
    }

    service.run();

    return 0;
}
