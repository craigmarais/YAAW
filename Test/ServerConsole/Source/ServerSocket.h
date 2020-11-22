#pragma once
#include "SocketLib.h"
#include <iostream>
#include <string>
#include "Person.h"

class ServerSocket : public SocketLib::TcpServer
{
public:
    ServerSocket(int port) : TcpServer(port)
    { }

    virtual ~ServerSocket() = default;

protected:
    void on_new_connection(const std::shared_ptr<SocketLib::TcpConnection> new_connection) override
    {
        std::cout << new_connection->endpoint << " Connected" << std::endl;
    }

    void on_message_received(const std::shared_ptr<SocketLib::PacketData>& packet) override
    {
        //std::cout << "Message recieved from connection: " << packet->endpoint << std::endl;

        auto* const person = reinterpret_cast<Person*>(packet->data.get());
        const std::string name(person->name.begin(), std::find(person->name.begin(), person->name.end(), '\0'));
        //std::cout << "id: " << person->id << ", name: " << name << std::endl;

        write(packet);
    }
    void on_message_sent(const std::shared_ptr<SocketLib::PacketData>& data) override
    {
        //std::cout << "Message sent to connection: " << data->endpoint << std::endl;
    }
};
