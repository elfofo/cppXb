cmake_minimum_required(VERSION 2.8)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
set(CMAKE_FIND_LIBRARY_PREFIXES lib)

#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_STATIC_RUNTIME OFF)
#set(Boost_USE_MULTITHREADED ON)

#set(BOOST_ROOT /usr/local/include/)
#set(BOOST_LIBRARYDIR /usr/local/lib/)

#find_package(Boost REQUIRED COMPONENTS system)

include_directories(gen/includes)

project(cppXbTest)


file(GLOB cppXbTest_SRC "test.cpp" "gen/src/*.cpp")

add_executable(cppXbTest ${cppXbTest_SRC})
#target_link_libraries(cppXbTest ${Boost_LIBRARIES})

set_target_properties(cppXbTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(cppXbTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(cppXbTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
