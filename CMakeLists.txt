cmake_minimum_required(VERSION 3.20)


project(skiplist)


set(CMAKE_CXX_STANDARD 17)

# if(CMAKE_BUILD_TYPE_DEBUG)
add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address -fsanitize=leak -fsanitize=undefined)
# endif()



add_executable(main main.cc SkipList.hpp)