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