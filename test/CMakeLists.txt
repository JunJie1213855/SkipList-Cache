# GoogleTest setup
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(skiplist_test
    main.cpp
    skiplist_test.cpp
)

target_include_directories(skiplist_test PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(skiplist_test
    GTest::gtest_main
    pthread
)

# Register tests
include(GoogleTest)
gtest_discover_tests(skiplist_test)
