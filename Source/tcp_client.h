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
