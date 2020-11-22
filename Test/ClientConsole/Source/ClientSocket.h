#pragma once
#include "SocketLib.h"
#include <iostream>

class ClientSocket : public SocketLib::TcpClient
{
public:
    ClientSocket() : TcpClient()
    { }

    virtual ~ClientSocket() = default;

protected:
    void on_connected(const std::shared_ptr<SocketLib::TcpConnection> new_connection) override
    {
        std::cout << "Connected to: " << new_connection->endpoint << std::endl;
    }
    void on_message_received(const std::shared_ptr<SocketLib::PacketData>& packet) override
    {
        //std::cout << "Message recieved from connection: " << packet->endpoint << std::endl;
    }
    void on_message_sent(const std::shared_ptr<SocketLib::PacketData>& data) override
    {
        //std::cout << "Message sent to connection: " << data->endpoint << std::endl;
    }
};
