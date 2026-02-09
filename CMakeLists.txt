cmake_minimum_required(VERSION 3.29)

project(Interrupt-Driver LANGUAGES C CXX)

option(ENABLE_TESTS "Build the test suite" OFF)

find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        asio-1-36-0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_MakeAvailable(asio)

add_library(Interrupt-Driver STATIC
    src/Interrupt/Controller.cpp
    src/Interrupt/Listener.cpp
    src/Interrupt/InterruptNodeFinder.cpp
    src/Interrupt/ScopedFd.cpp
)
target_include_directories(Interrupt-Driver PRIVATE include)
target_include_directories(Interrupt-Driver PUBLIC ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(Interrupt-Driver PUBLIC ASIO_STANDALONE)
target_link_libraries(Interrupt-Driver PRIVATE Threads::Threads)

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMAND ${CMAKE_COMMAND} -E create_symlink 
                "${CMAKE_BINARY_DIR}/compile_commands.json" 
                "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMENT "Linking compile_commands.json to source root"
    )
    # 创建一个伪目标确保它执行
    add_custom_target(link_compile_commands ALL 
        DEPENDS "${CMAKE_SOURCE_DIR}/compile_commands.json")
endif()