cmake_minimum_required(VERSION 3.20)

project(skiplist)

# support for optional
set(CMAKE_CXX_STANDARD 17)

# Enable testing
enable_testing()

# debug type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("BUILD WITH DEBUG")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=leak -fsanitize=undefined)
    # DEBUG 模式下启用调试输出宏
    message("ADD #SKIP_DEBUG_OUTSTREAM")
    add_compile_definitions(SKIP_DEBUG_OUTSTREAM)
endif()

# for test
add_executable(main
    main.cc SkipList.hpp
)

# benchmark
add_executable(benchmark
    benchmark.cc SkipList.hpp
)

# cli
add_executable(cli
    cli.cc SkipList.hpp
)
target_link_libraries(cli readline)

# Unit tests
add_subdirectory(test)