#pragma once
#include <array>

#pragma pack(1)
struct Person
{
    int id;
    std::array<char, 100> name;
};
#pragma pack()