#include "ClientSocket.h"

int main()
{
    ClientSocket cs;
    cs.connect("127.0.0.1", 9991);

    std::getchar();
}
