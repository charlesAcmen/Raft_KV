cmake_minimum_required(VERSION 3.10)
project(craft-kv C CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(spdlog REQUIRED)


file(GLOB SOURCE_FILES
    "src/*.cpp"
)
file(GLOB HEADER_FILES
    "include/*.h"
)
add_executable(craft-kv ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(craft-kv PRIVATE {CMAKE_SOURCE_DIR}/include)
target_link_libraries(craft-kv PRIVATE spdlog::spdlog)