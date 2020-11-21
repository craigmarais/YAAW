cmake_minimum_required(VERSION 3.10)

project(SocketLib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -DASIO_STANDALONE")

include_directories("include")

add_subdirectory(Test)

file(GLOB SocketLibSource
    "Source/*.cpp"
    "Source/*.h"
    "Source/Structs/*.h"
)

file(GLOB SourceFiles
    "Source/*.cpp"
)

file(GLOB HeaderFiles
    "Source/*.h"
)

file(GLOB StructsFiles
	"Source/Structs/*.h"
)

source_group("HeaderFiles\\${HeaderFiles}")
source_group("SourceFiles\\${SourceFiles}")
source_group("Structs\\${StructsFiles}")

add_library(SocketLib ${SocketLibSource})
set_target_properties(SocketLib PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(SocketLib -lpthread)

file(STRINGS "Source/Constant.h" Constant_H)
file(STRINGS "Source/TcpClient.h" TcpClient_H)
file(STRINGS "Source/TcpConnection.h" TcpConnection_H)
file(STRINGS "Source/TcpServer.h" TcpServer_H)
file(STRINGS "Source/Structs/PacketData.h" PacketData_H)

list(APPEND SOCKETLIB_SINGLE
    "${Constant_H}"
	"${TcpClient_H}"
	"${TcpConnection_H}"
	"${TcpServer_H}"
	"${PacketData_H}"
)

file(REMOVE "include/SocketLib.h")
foreach(LINE IN LISTS SOCKETLIB_SINGLE)
    #if(LINE MATCHES "#include \"")
    #   file(APPEND "include/SocketLib.h" "// ${LINE}\n")
    #else()
        file(APPEND "include/SocketLib.h" "${LINE}\n")
    #endif()
endforeach()