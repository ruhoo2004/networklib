cmake_minimum_required (VERSION 2.6)
set (CMAKE_CXX_STANDARD 11)
project (tcpdemo)

include_directories("/usr/include")
link_directories("/usr/lib" "/usr/lib/x86_64-linux-gnu")
aux_source_directory(. DIR_LIB_SRCS)

add_library(TCPConnection ${DIR_LIB_SRCS})

add_executable(server server.cpp)
target_link_libraries(server TCPConnection event)

add_executable(client client.cpp)
target_link_libraries(client TCPConnection event)

add_executable(epollserver epollserver.cpp)

add_executable(asioserver asioserver.cpp)
target_link_libraries(asioserver boost_system)

