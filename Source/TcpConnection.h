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
            endpoint = socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port());

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

        ~TcpConnection()
        {
            shutdown = true;
            if (read_perf_thread.joinable())
                read_perf_thread.join();
            if (write_perf_thread.joinable())
                write_perf_thread.join();
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

        void write(const std::shared_ptr<PacketData>& packet)
        {
            asio::write(*socket, asio::buffer(packet->data.get(), packet->length));
            message_sent_callback(packet);
            write_counter++;
        }

        std::function<void(const std::shared_ptr<PacketData>&)> message_received_callback;
        std::function<void(const std::shared_ptr<PacketData>&)> message_sent_callback;
        std::string endpoint;

    private:
        void read_body(int length)
        {
            socket->async_read_some(asio::buffer(read_buffer, length),
                [&](const asio::error_code ec, size_t _length)
                {
                    if (!ec)
                    {
                        std::vector<unsigned char> buffer(std::begin(read_buffer), std::end(read_buffer));
                        auto endpoint = socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port());
                        const auto packet_data = std::make_shared<PacketData>(buffer.data(), _length, endpoint);
                        read_counter++;
                        message_received_callback(packet_data);
                    }
                });
            start_reading();
        }

        std::shared_ptr<asio::ip::tcp::socket> socket;
        unsigned char read_buffer[MAX_MESSAGE_SIZE];
        bool shutdown = false;

        int write_counter = 0;
        std::thread write_perf_thread;
        int read_counter = 0;
        std::thread read_perf_thread;
    };
}
#endif
