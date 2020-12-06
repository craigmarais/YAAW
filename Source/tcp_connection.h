#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <functional>
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <queue>


#include "Constant.h"
#include "Structs/packet_data.h"
#include "concurrentqueue.h"

namespace sl
{
    class tcp_connection
    {
    public:
        tcp_connection(std::shared_ptr<asio::ip::tcp::socket> socket_)
            : socket(std::move(socket_))
        {
            endpoint = socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port());
            read_thread = std::thread(&tcp_connection::read_thread_runner, this);
            write_thread = std::thread(&tcp_connection::write_thread_runner, this);
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

        void write(const std::shared_ptr<packet_data>& packet)
        {
            if (write_queue.size_approx() < 1'000'000)
                write_queue.enqueue(packet);
        }

        std::function<void(const std::shared_ptr<packet_data>&)> message_received_callback;
        std::function<void(const std::shared_ptr<packet_data>&)> message_sent_callback;
        std::string endpoint;

        moodycamel::ConcurrentQueue<std::shared_ptr<packet_data>> read_queue;
        moodycamel::ConcurrentQueue<std::shared_ptr<packet_data>> write_queue;

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
                        const auto packet = std::make_shared<packet_data>(buffer.data(), _length, endpoint);
                        {
                            read_queue.enqueue(packet);
                        }
                    }
                });
            start_reading();
        }

        void read_thread_runner()
        {
            while(!shutdown)
            {
                std::shared_ptr<packet_data> packet;
                while(read_queue.try_dequeue(packet))
                {
                    message_received_callback(packet);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }

        void write_thread_runner()
        {
            while (!shutdown)
            {
                std::shared_ptr<packet_data> packet;
                while (write_queue.try_dequeue(packet))
                {
                    asio::write(*socket, asio::buffer(packet->data.get(), packet->length));
                    message_sent_callback(packet);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }

        std::shared_ptr<asio::ip::tcp::socket> socket;
        unsigned char read_buffer[MAX_MESSAGE_SIZE];

        std::thread read_thread;
        std::thread write_thread;
        bool shutdown = false;
    };
}
#endif
