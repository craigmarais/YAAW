#include "ServerSocket.h"
#include <iostream>
#include <functional>


int main()
{
    ServerSocket ss(9991);
    ss.run();

    std::cout << "<Press any key to exit.\n";
    getchar();
}
