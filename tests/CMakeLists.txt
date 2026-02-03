FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_controller Interrupt/test_controller.cpp)

target_link_libraries(test_controller 
    PRIVATE 
        Interrupt-Driver 
        GTest::gtest_main
)

# We need to include the directories from the main library to access private headers if necessary, 
# or just the public interface. Since we are testing internal logic which might rely on layout, 
# ensuring we have the same include paths is good. 
# However, Interrupt-Driver uses 'include' as a private include directory. 
# For unit tests often we want to test public API, but here we might need access to definitions.
# The root CMakeLists.txt adds 'include' as PRIVATE to Interrupt-Driver.
# We should add it here too so the test file can find headers.
target_include_directories(test_controller PRIVATE ${CMAKE_SOURCE_DIR}/include)

include(GoogleTest)
gtest_discover_tests(test_controller)
