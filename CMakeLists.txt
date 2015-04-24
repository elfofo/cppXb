cmake_minimum_required(VERSION 2.8)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
set(CMAKE_FIND_LIBRARY_PREFIXES lib)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_MULTITHREADED ON)

#set(BOOST_ROOT /home/fofo/Downloads/boost_1_58_0/)
set(BOOST_LIBRARYDIR /usr/local/lib/)

find_package(Boost REQUIRED COMPONENTS system filesystem)

message("libs ${Boost_LIBRARIES}")

include_directories(${Boost_INCLUDE_DIRS} src)

link_directories(/usr/local/lib)

project(progentest)


file(GLOB cppXb_SRC "src/*.h" "src/*.cpp")

add_executable(cppXb ${cppXb_SRC})
target_link_libraries(cppXb ${Boost_LIBRARIES})
set_target_properties(cppXb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(cppXb PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(cppXb PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
