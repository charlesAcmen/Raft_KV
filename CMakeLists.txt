cmake_minimum_required(VERSION 3.10)
project(craft-kv C CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(spdlog REQUIRED)


file(GLOB SOURCE_FILES
    "src/*.cpp"
    "src/rpc/*.cpp"
    "src/raft/*.cpp"
)

#static library:.a or .lib
add_library(raft_lib 
${SOURCE_FILES} 
#.h files will not be compiled separately
)

#header file searching path
target_include_directories(raft_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(craft-kv src/main.cpp)
target_link_libraries(craft-kv PRIVATE raft_lib spdlog::spdlog)

# file(COPY ${CMAKE_SOURCE_DIR}/data/ DESTINATION ${CMAKE_BINARY_DIR}/data/)