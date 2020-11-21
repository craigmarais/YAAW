file(GLOB ServerConsoleSource
    "Source/*.cpp"
    "Source/*.h"
)

include_directories("../../Source")

add_executable(ServerConsole ${ServerConsoleSource})

target_link_libraries(ServerConsole)
