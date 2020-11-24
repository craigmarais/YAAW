#ifndef CONSTANT_H
#define CONSTANT_H

namespace sl
{
    const int LISTEN_PORT = 5543;
    const int NUM_ACCEPTING_THREADS = 2;
    constexpr size_t MAX_MESSAGE_SIZE = 1024;
}
#endif
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include <iostream>
#include <string>
#include "asio.hpp"
#include "Structs/packet_data.h"
#include "tcp_connection.h"

namespace sl
{
    class tcp_client
    {
    public:
        tcp_client() = default;
        virtual ~tcp_client() = default;

        void connect(const std::string& address, const unsigned short port)
        {
            socket = std::make_shared<asio::ip::tcp::socket>(context);
            asio::ip::tcp::resolver resolver(context);
            asio::connect(*socket, resolver.resolve(address, std::to_string(port)));
            server_connection = std::make_shared<tcp_connection>(socket);
            server_connection->message_received_callback = [this](const std::shared_ptr<packet_data>& packet) { on_message_received(packet); };
            server_connection->message_sent_callback = [this](const std::shared_ptr<packet_data>& packet) { on_message_sent(packet); };
            server_connection->start_reading();
            service_thread = std::thread([&]() {context.run(); });
            on_connected(server_connection);
        }

        void send(const std::shared_ptr<packet_data>& packet)
        {
            server_connection->write(packet);
            on_message_sent(packet);
        }

        [[nodiscard]] std::string server_endpoint() const
        {
            return server_connection->endpoint;
        }

    protected:
        virtual void on_connected(const std::shared_ptr<tcp_connection> new_connection)
        {
            std::cout << "Connected to server in base.\n";
        }

        virtual void on_message_received(const std::shared_ptr<packet_data>& data)
        {  }

        virtual void on_message_sent(const std::shared_ptr<packet_data>& data)
        {  }

        virtual void on_error(const std::runtime_error& error)
        {
            std::cout << "An error has occurred in base.\n";
        }
        [[nodiscard]] size_t read_queue_size()
        {
            return server_connection->read_queue.size();
        }

        [[nodiscard]] size_t write_queue_size()
        {
            return server_connection->write_queue.size();
        }

    private:
        asio::io_context context;
        std::shared_ptr<asio::ip::tcp::socket> socket;
        std::shared_ptr<tcp_connection> server_connection;
        std::thread service_thread;
    };
}
#endif
#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <functional>
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <queue>


#include "Constant.h"
#include "Structs/packet_data.h"
#include "concurrent_queue.h"

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
            std::lock_guard<std::mutex> lock(write_mutex);
            write_queue.push(packet);
        }

        std::function<void(const std::shared_ptr<packet_data>&)> message_received_callback;
        std::function<void(const std::shared_ptr<packet_data>&)> message_sent_callback;
        std::string endpoint;

        std::queue<std::shared_ptr<packet_data>> read_queue;
        std::queue<std::shared_ptr<packet_data>> write_queue;

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
                            std::lock_guard<std::mutex> lock(read_mutex);
                            read_queue.push(packet);
                        }
                    }
                });
            start_reading();
        }

        void read_thread_runner()
        {
            while(!shutdown)
            {
                if (read_queue.empty())
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    continue;
                }
                while(!read_queue.empty())
                {
                    std::lock_guard<std::mutex> lock(read_mutex);
                    auto packet = read_queue.front();
                    message_received_callback(packet);
                    read_queue.pop();
                }
            }
        }

        void write_thread_runner()
        {
            while (!shutdown)
            {
                if (write_queue.empty())
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    continue;
                }
                while (!write_queue.empty())
                {
                    std::lock_guard<std::mutex> lock(write_mutex);
                    auto packet = write_queue.front();

                    asio::write(*socket, asio::buffer(packet->data.get(), packet->length));
                    message_sent_callback(packet);

                    write_queue.pop();
                }
            }
        }

        std::shared_ptr<asio::ip::tcp::socket> socket;
        unsigned char read_buffer[MAX_MESSAGE_SIZE];

        std::mutex read_mutex;
        std::thread read_thread;

        std::mutex write_mutex;
        std::thread write_thread;
        bool shutdown = false;
    };
}
#endif
#ifndef SOCKET_LIB
#define SOCKET_LIB
#include "tcp_connection.h"
#include "Structs/packet_data.h"

#include <map>
#include <asio.hpp>
#include <iostream>


namespace sl
{
    class tcp_server
    {
    public:
        tcp_server(int port)
            : acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
        {}
        virtual ~tcp_server() {}

        void run()
        {
            service_thread = std::thread([&]() {begin_accept(); });
            on_started(acceptor.local_endpoint().address().to_string(), acceptor.local_endpoint().port());
        }

        void write(const std::shared_ptr<packet_data>& packet)
        {
            connected_clients[packet->endpoint]->write(packet);
        }

        [[nodiscard]] std::map<std::string, std::shared_ptr<tcp_connection>> clients() const
        {
            return connected_clients;
        }

    protected:
        virtual void on_new_connection(const std::shared_ptr<tcp_connection> new_connection)
        {
            std::cout << "new connection in base.\n";
        }

        virtual void on_message_received(const std::shared_ptr<packet_data>& data)
        {}

        virtual void on_message_sent(const std::shared_ptr<packet_data>& data)
        {}

        virtual void on_error(const std::runtime_error& error)
        {
            std::cout << "An error has occurred in base. error: " << error.what() << "\n";
        }

        virtual void on_started(const std::string& address, const unsigned short port)
        {
            std::cout << "Server started on: " << address << ":" << port << std::endl;
        }

        [[nodiscard]] size_t read_queue_size(std::string endpoint)
        {
            return connected_clients[endpoint]->read_queue.size();
        }

        [[nodiscard]] size_t write_queue_size(std::string endpoint)
        {
            return connected_clients[endpoint]->write_queue.size();
        }
        std::map<std::string, std::shared_ptr<tcp_connection>> connected_clients;

    private:
        void begin_accept()
        {
            auto socket = std::make_shared<asio::ip::tcp::socket>(context);
            acceptor.async_accept(*socket, [&](const asio::error_code ec) { if (!ec) accept_client(socket); });
            context.run();
        }

        void accept_client(const std::shared_ptr<asio::ip::tcp::socket>& client_socket)
        {
            client_socket->set_option(asio::ip::tcp::no_delay(true));

            auto tcp_client = std::make_shared<tcp_connection>(client_socket);
            tcp_client->message_received_callback = [this](const std::shared_ptr<packet_data>& data) { on_message_received(data); };
            tcp_client->message_sent_callback = [this](const std::shared_ptr<packet_data>& packet) { on_message_sent(packet); };
            tcp_client->start_reading();
            connected_clients.emplace(tcp_client->endpoint, tcp_client);

            on_new_connection(tcp_client);
            begin_accept();
        }

        asio::io_context context;
        asio::ip::tcp::acceptor acceptor;

        std::thread service_thread;
    };
}
#endif
#ifndef PACKET_DATA_H
#define PACKET_DATA_H
#include <cstring>
#include <utility>

namespace sl
{
    struct packet_data
    {
        std::unique_ptr<unsigned char[]> data;
        size_t length = 0;
        std::string endpoint;

        packet_data() = default;

        packet_data(const int _length)
            : length(_length)
        {}

        packet_data(unsigned char* _data, const int _length, std::string _endpoint)
            : length(_length),
              endpoint(std::move(_endpoint))
        {
            data = std::make_unique<unsigned char[]>(length);
            memcpy(data.get(), _data, length);
        }

        void set_length(int length_)
        {
            length = length_;
        }
    };
}
#endif
