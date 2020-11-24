#include "ClientSocket.h"
#include "Person.h"

int main()
{
    ClientSocket cs;
    cs.connect("127.0.0.1", 9991);

    while (true)
    {
        Person p{};
        p.id = 1;
        p.name = { 'C','r','a','i','g',' ','M','a','r','a','i','s' };

        unsigned char packet[sizeof(Person) + sizeof(int)];
        auto length = sizeof(Person);
        memcpy(&packet[0], &length, sizeof(int));
        memcpy(&packet[sizeof(int)], &p, sizeof(Person));

        const auto data = std::make_shared<sl::packet_data>(packet, sizeof(p) + sizeof(int), cs.server_endpoint());

        cs.send(data);
    }
    std::getchar();
}
