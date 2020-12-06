#pragma once
#include "SocketLib.h"
#include <iostream>
#include <string>
#include "Person.h"

class ServerSocket : public sl::tcp_server
{
public:
    ServerSocket(int port) : tcp_server(port)
    {
        write_perf_thread = std::thread([&]()
            {
                while (!shutdown)
                {
                    if (write_counter != 0)
                    {
                        std::cout << "Write: " << write_counter << " /s. Stack: " <<  write_queue_size(connected_clients.begin()->second->endpoint) << "\n";
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
                        std::cout << "Read: " << read_counter << " /s. Stack: " << read_queue_size(connected_clients.begin()->second->endpoint) << "\n";
                        read_counter = 0;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });
    }

    virtual ~ServerSocket()
    {
        shutdown = true;
        if (read_perf_thread.joinable())
            read_perf_thread.join();
        if (write_perf_thread.joinable())
            write_perf_thread.join();
    }

protected:
    void on_started(const std::string& address, const unsigned short port) override
    {
        std::cout << "Server socket is listening on: " << address << ":" << std::to_string(port) << std::endl;
    }
    void on_new_connection(const std::shared_ptr<sl::tcp_connection> new_connection) override
    {
        std::cout << new_connection->endpoint << " Connected" << std::endl;
    }

    void on_message_received(const std::shared_ptr<sl::packet_data>& packet) override
    {
        read_counter++;
        //auto* const person = reinterpret_cast<Person*>(packet->data.get());
        //const std::string name(person->name.begin(), std::find(person->name.begin(), person->name.end(), '\0'));
        //std::cout << "id: " << person->id << ", name: " << name << std::endl;

        write(packet);
    }
    void on_message_sent(const std::shared_ptr<sl::packet_data>& data) override
    {
        write_counter++;
    }

private:
    int write_counter = 0;
    std::thread write_perf_thread;
    int read_counter = 0;
    std::thread read_perf_thread;
    bool shutdown = false;
};
