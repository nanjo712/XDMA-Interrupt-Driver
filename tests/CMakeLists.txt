FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(driver_test
    test_main.cpp
    Interrupt/test_controller.cpp
    Interrupt/test_listener.cpp
)

target_link_libraries(driver_test PRIVATE Interrupt-Driver GTest::gtest GTest::gmock)
target_include_directories(driver_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
