#ifndef CONSTANT_H
#define CONSTANT_H

namespace SocketLib
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
#include "Structs/PacketData.h"
#include "TcpConnection.h"

namespace SocketLib
{
    class TcpClient
    {
    public:
        TcpClient() = default;
        virtual ~TcpClient() = default;

        void connect(const std::string& address, const unsigned short port)
        {
            auto socket = std::make_shared<asio::ip::tcp::socket>(context);
            asio::ip::tcp::resolver resolver(context);
            asio::connect(*socket, resolver.resolve(address, std::to_string(port)));
            server_connection = std::make_shared<TcpConnection>(std::move(socket));
            on_connected(server_connection);
            server_connection->message_received_callback = [&](const std::shared_ptr<PacketData>& packet) { on_message_received(packet); };
            server_connection->start_reading();
        }

        void send(const std::shared_ptr<PacketData>& data)
        {
            server_connection->write(data->data.get(), data->length);
            on_message_sent(data);
        }

    protected:
        virtual void on_connected(const std::shared_ptr<TcpConnection> new_connection)
        {
            std::cout << "Connected to server in base.\n";
        }

        virtual void on_message_received(const std::shared_ptr<PacketData>& data)
        {  }

        virtual void on_message_sent(const std::shared_ptr<PacketData>& data)
        {  }

        virtual void on_error(const std::runtime_error& error)
        {
            std::cout << "An error has occurred in base.\n";
        }

    private:
        asio::io_context context;
        std::shared_ptr<TcpConnection> server_connection;
    };
}
#endif
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
#ifndef SOCKET_LIB
#define SOCKET_LIB
#include "TcpConnection.h"
#include "Structs/PacketData.h"

#include <map>
#include <asio.hpp>
#include <iostream>


namespace SocketLib
{
    class TcpServer
    {
    public:
        TcpServer(int port)
            : acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
            client_id_count(0)
        {}
        virtual ~TcpServer() {}

        void run()
        {
            service_thread = std::thread([&]() {begin_accept(); });
            on_started(acceptor.local_endpoint().address().to_string(), acceptor.local_endpoint().port());
        }

    protected:
        virtual void on_new_connection(const std::shared_ptr<TcpConnection> new_connection)
        {
            std::cout << "new connection in base.\n";
        }

        virtual void on_message_received(const std::shared_ptr<PacketData>& data)
        {}

        virtual void on_message_sent(const std::shared_ptr<PacketData>& data)
        {}

        virtual void on_error(const std::runtime_error& error)
        {
            std::cout << "An error has occurred in base. error: " << error.what() << "\n";
        }

        virtual void on_started(const std::string& address, const unsigned short port)
        {
            std::cout << "Server started on: " << address << ":" << port << std::endl;
        }

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

            auto tcp_client = std::make_shared<TcpConnection>(client_socket);
            tcp_client->message_received_callback = [this](const std::shared_ptr<PacketData>& data) { on_message_received(data); };
            tcp_client->start_reading();
            connected_clients.emplace(++client_id_count, tcp_client);

            on_new_connection(tcp_client);
            begin_accept();
        }

        asio::io_context context;
        asio::ip::tcp::acceptor acceptor;
        std::map<int, std::shared_ptr<TcpConnection>> connected_clients;

        std::thread service_thread;

        int client_id_count;
    };
}
#endif
#ifndef PACKET_DATA_H
#define PACKET_DATA_H
#include <cstring>

namespace SocketLib
{
    struct PacketData
    {
        std::unique_ptr<unsigned char[]> data;
        size_t length = 0;

        PacketData() = default;

        PacketData(const int _length)
            : length(_length)
        {}

        PacketData(unsigned char* _data, const int _length)
            : length(_length)
        {
            data = std::make_unique<unsigned char[]>(length);
            memcpy(data.get(), _data, length);
        }

        ~PacketData() = default;

        void set_length(int length_)
        {
            length = length_;
        }
    };
}
#endif