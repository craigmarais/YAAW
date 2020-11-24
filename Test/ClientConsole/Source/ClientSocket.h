#pragma once
#include "SocketLib.h"
#include <iostream>

class ClientSocket : public sl::tcp_client
{
public:
    ClientSocket() : tcp_client()
    {
        write_perf_thread = std::thread([&]()
            {
                while (!shutdown)
                {
                    if (write_counter != 0)
                    {
                        std::cout << "Write: " << write_counter << " /s. Stack: " << write_queue_size() << "\n";
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
                        std::cout << "Read: " << read_counter << " /s. Stack: " << read_queue_size() << "\n";
                        read_counter = 0;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });
    }

    virtual ~ClientSocket()
    {
        shutdown = true;
        if (read_perf_thread.joinable())
            read_perf_thread.join();
        if (write_perf_thread.joinable())
            write_perf_thread.join();
    }

protected:
    void on_connected(const std::shared_ptr<sl::tcp_connection> new_connection) override
    {
        std::cout << "Connected to: " << new_connection->endpoint << std::endl;
    }
    void on_message_received(const std::shared_ptr<sl::packet_data>& packet) override
    {
        read_counter++;
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
