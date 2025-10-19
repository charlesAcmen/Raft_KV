cmake_minimum_required(VERSION 3.10)
project(craft-kv C CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(spdlog REQUIRED)


file(GLOB SOURCE_FILES
    "src/rpc/*.cpp"
    "src/raft/*.cpp"
)

# static library:.a or .lib
# .h files will not be compiled separately
add_library(raft_lib 
${SOURCE_FILES} 
)

#header file searching path
target_include_directories(raft_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)



add_executable(craft-kv src/main.cpp)
target_link_libraries(craft-kv PRIVATE raft_lib spdlog::spdlog)



# enable_testing()
# add_executable(raft_test test/raft_conflict_test.cpp)
# target_link_libraries(raft_test PRIVATE raft_lib gtest gtest_main pthread spdlog::spdlog)
# add_test(NAME RaftConflictTest COMMAND raft_test)