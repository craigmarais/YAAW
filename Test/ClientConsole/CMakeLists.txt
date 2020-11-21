file(GLOB ClientConsoleSource
    "Source/*.cpp"
    "Source/*.h"
)

include_directories("../../Source")

add_executable(ClientConsole ${ClientConsoleSource})

target_link_libraries(ClientConsole)
