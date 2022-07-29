cmake_minimum_required(VERSION 3.5.1)

project(HideAndSeek C CXX)

option(DEV  "set on for release")
# set(DEV OFF)
if(DEV)
    message("Compile in Dev mode")
else()
    message("Compile in Release mode")
    set(CMAKE_BUILD_TYPE "Release")
endif()

include_directories(${CMAKE_BINARY_DIR}/proto) # so that every code can use the grpc generated files
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(proto)
add_subdirectory(client)
add_subdirectory(ai)

add_executable(main main.cpp)
target_link_libraries(main AI Client)

# add yaml-cpp as dependency
find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIRS})
target_link_libraries(main ${YAML_CPP_LIBRARIES})
