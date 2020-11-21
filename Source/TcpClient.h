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
