cmake_minimum_required(VERSION 3.16)
project(cosm CXX)

add_compile_options(-Wno-deprecated-declarations)

set(BOOST_INCLUDE_LIBRARIES system asio circular_buffer)
add_subdirectory(boost)

set(BOOST_ASYNC_BUILD_EXAMPLES OFF)
set(BOOST_ASYNC_BUILD_TESTS OFF)
add_subdirectory(boost-async)

set(BOOST_SAM_BUILD_INLINE ON)
add_subdirectory(boost-sam)

set(CMAKE_CXX_STANDARD 23)
add_executable(player player.cpp)
target_include_directories(player PRIVATE include)
target_link_libraries(player PRIVATE Boost::async)

add_executable(cancel cancel.cpp)
target_include_directories(cancel PRIVATE include)
target_link_libraries(cancel PRIVATE Boost::async Boost::sam)
