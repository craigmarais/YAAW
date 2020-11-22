#ifndef PACKET_DATA_H
#define PACKET_DATA_H
#include <cstring>
#include <utility>

namespace SocketLib
{
    struct PacketData
    {
        std::unique_ptr<unsigned char[]> data;
        size_t length = 0;
        std::string endpoint;

        PacketData() = default;

        PacketData(const int _length)
            : length(_length)
        {}

        PacketData(unsigned char* _data, const int _length, std::string _endpoint)
            : length(_length),
              endpoint(std::move(_endpoint))
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
