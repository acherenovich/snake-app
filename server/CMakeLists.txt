cmake_minimum_required(VERSION 3.29)
project(server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_FIND_FRAMEWORK NEVER)
set(SFML_STATIC_LIBRARIES ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(OpenGL REQUIRED)
find_package(SFML 3 COMPONENTS Network System REQUIRED)

include_directories(src)
include_directories(includes)
include_directories(interfaces)
include_directories(/usr/local/include)

file(GLOB_RECURSE CPP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HPP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
add_executable(server ${CPP} ${HPP})

target_link_libraries(server PRIVATE SFML::Network SFML::System)

#macro(print_all_variables)
#    message(STATUS "print_all_variables------------------------------------------{")
#    get_cmake_property(_variableNames VARIABLES)
#    foreach (_variableName ${_variableNames})
#        message(STATUS "${_variableName}=${${_variableName}}")
#    endforeach()
#    message(STATUS "print_all_variables------------------------------------------}")
#endmacro()
#
#print_all_variables()
