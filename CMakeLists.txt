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

add_executable(Interrupt-Driver-Example src/Example/example_main.cpp)
target_include_directories(Interrupt-Driver-Example PRIVATE include)
target_link_libraries(Interrupt-Driver-Example PRIVATE Interrupt-Driver)

add_executable(Interrupt-Driver-Trigger src/Example/trigger.cpp)
target_include_directories(Interrupt-Driver-Trigger PRIVATE include)
target_link_libraries(Interrupt-Driver-Trigger PRIVATE Interrupt-Driver)

add_executable(Interrupt-Driver-WriteMailbox src/Example/write_mailbox.cpp)
target_include_directories(Interrupt-Driver-WriteMailbox PRIVATE include)
target_link_libraries(Interrupt-Driver-WriteMailbox PRIVATE Interrupt-Driver)

if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

set(SOURCE_JSON "${CMAKE_BINARY_DIR}/compile_commands.json")
set(TARGET_JSON "${CMAKE_SOURCE_DIR}/compile_commands.json")

if(EXISTS "${SOURCE_JSON}" OR NOT EXISTS "${TARGET_JSON}")
    message(STATUS "Linking compile_commands.json to source root...")
    
    file(CREATE_LINK "${SOURCE_JSON}" "${TARGET_JSON}" SYMBOLIC COPY_ON_ERROR)
    
endif()