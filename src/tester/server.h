//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// copyright (c) 2003-2008 christopher M. kohlhoff (chris at kohlhoff dot com)
//
// distributed under the boost software license, version 1.0. (see accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "xi/ext/configure.h"

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

using namespace boost::asio;
using namespace xi;
using ip::tcp;

class session {
public:
  session(io_service &io_service) : socket_(io_service) {}

  tcp::socket &socket() { return socket_; }

  void start() { schedule_read(); }

  void schedule_read() {
    async_read(socket_, buffer(data_, 0),
               boost::bind(&session::handle_read_event, this,
                           placeholders::error,
                           placeholders::bytes_transferred));
  }
  void handle_read_event(boost::system::error_code error,
                         size_t bytes_transferred) {
    while (!error) {
      auto data = make_shared< vector< char > >(max_length);
      bytes_transferred = socket_.receive(buffer(*data), MSG_DONTWAIT, error);
      if (bytes_transferred > 0) {
        async_write(socket_, buffer(*data, bytes_transferred),
                    [data](auto error, size_t) {});
      }
      if (bytes_transferred < max_length) {
        if (!error) { schedule_read(); }
        break;
      }
    }
    if (error) { delete this; }
  }

private:
  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server {
public:
  server(io_service &io_service, short port)
      : io_service_(io_service)
      , acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
    session *new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&server::handle_accept, this,
                                       new_session, placeholders::error));
  }

  void handle_accept(session *new_session,
                     const boost::system::error_code &error) {
    if (!error) {
      new_session->start();
      new_session = new session(io_service_);
      acceptor_.async_accept(new_session->socket(),
                             boost::bind(&server::handle_accept, this,
                                         new_session, placeholders::error));
    } else { delete new_session; }
  }

private:
  io_service &io_service_;
  tcp::acceptor acceptor_;
};
