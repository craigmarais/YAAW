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
        std::cout << new_connection->address << ":" << new_connection->port << " Connected" << std::endl;
    }
    void on_message_received(const std::shared_ptr<SocketLib::PacketData>& packet) override
    {
        std::cout << "Message recieved in derived." << std::endl;
    }
};
