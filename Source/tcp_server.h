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