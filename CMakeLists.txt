cmake_minimum_required(VERSION 3.16)
project(cosm CXX)

set(BOOST_INCLUDE_LIBRARIES system asio circular_buffer)
add_subdirectory(boost)

set(BOOST_ASYNC_BUILD_EXAMPLES OFF)
set(BOOST_ASYNC_BUILD_TESTS OFF)
add_subdirectory(boost-async)

set(CMAKE_CXX_STANDARD 23)
add_executable(player player.cpp)
target_include_directories(player PRIVATE include)
target_link_libraries(player PRIVATE Boost::async)
