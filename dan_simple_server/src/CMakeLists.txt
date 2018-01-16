cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_FLAGS "-std=c++11")
add_subdirectory(proto)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/proto/deps/protobuf/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_EXECUTABLE(ddd main.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/proto/lulu.pb.cc
 ${CMAKE_CURRENT_SOURCE_DIR}/proto/rpc.pb.cc
)
TARGET_LINK_LIBRARIES(ddd ${CMAKE_CURRENT_SOURCE_DIR}/proto/deps/protobuf/lib/libprotobuf.a)
