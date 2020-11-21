set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -DASIO_STANDALONE")

add_subdirectory(ClientConsole)
add_subdirectory(ServerConsole)