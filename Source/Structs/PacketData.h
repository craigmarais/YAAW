#ifndef PACKET_DATA_H
#define PACKET_DATA_H
#include <cstring>

namespace SocketLib
{
    struct PacketData
    {
        std::unique_ptr<unsigned char[]> data;
        size_t length = 0;

        PacketData() = default;

        PacketData(const int _length)
            : length(_length)
        {}

        PacketData(unsigned char* _data, const int _length)
            : length(_length)
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
