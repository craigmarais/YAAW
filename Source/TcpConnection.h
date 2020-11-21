#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <asio.hpp>

#include "Constant.h"
#include "Structs/PacketData.h"
#include "concurrent_queue.h"

namespace SocketLib
{
    class TcpConnection
    {
    public:
        TcpConnection(std::shared_ptr<asio::ip::tcp::socket> socket_)
            : socket(std::move(socket_))
        {
            port = socket->remote_endpoint().port();
            address = socket->remote_endpoint().address().to_string();
            write_perf_thread = std::thread([&]()
                {
                    while (!shutdown)
                    {
                        if (write_counter != 0)
                        {
                            std::cout << "Write: " << write_counter << " /s.\n";
                        }
                        write_counter = 0;
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                });
            read_perf_thread = std::thread([&]()
                {
                    while (!shutdown)
                    {
                        if (read_counter != 0)
                        {
                            std::cout << "Read: " << read_counter << " /s.\n";
                            read_counter = 0;
                        }
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                });
        }

        void start_reading()
        {
            socket->async_read_some(asio::buffer(read_buffer, 4),
                [&](asio::error_code ec, size_t bytes_transferred)
                {
                    if (!ec)
                    {
                        const auto length = read_buffer[0] + read_buffer[1] + read_buffer[2] + read_buffer[3];
                        read_body(length);
                    }
                });
        }

        void write(const unsigned char* packet, const int& length)
        {
            asio::error_code ec;
            asio::write(*socket, asio::buffer(packet, length), ec);
            write_counter++;
        }

        std::function<void(const std::shared_ptr<PacketData>&)> message_received_callback;
        std::string address;
        unsigned short port;

    private:
        void read_body(int length)
        {
            socket->async_read_some(asio::buffer(read_buffer, length),
                [&](const asio::error_code ec, size_t _length)
                {
                    if (!ec)
                    {
                        std::vector<unsigned char> buffer(std::begin(read_buffer), std::end(read_buffer));
                        const auto packet_data = std::make_shared<PacketData>(buffer.data(), _length);
                        message_received_callback(packet_data);
                        read_counter++;
                    }
                });
            start_reading();
        }

        std::shared_ptr<asio::ip::tcp::socket> socket;

        unsigned char read_buffer[MAX_MESSAGE_SIZE];

        std::unique_ptr<std::thread> socket_read_thread;

        int write_counter = 0;
        std::thread write_perf_thread;
        int read_counter = 0;
        std::thread read_perf_thread;

        bool shutdown = false;
    };
}
#endif
