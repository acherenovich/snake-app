cmake_minimum_required(VERSION 3.29)
project(snake)

option(BUILD_CLIENT "Build client instead of server" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_FIND_FRAMEWORK NEVER)
set(SFML_STATIC_LIBRARIES ON)
set(BUILD_SHARED_LIBS OFF)
set(ZLIB_USE_STATIC_LIBS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

IF (WIN32)
    set(SFML_DIR "C:/Libraries/SFML/SFML-2.6.2/lib/cmake/SFML")
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
ENDIF ()

add_subdirectory(third_party/zlib)

if(BUILD_CLIENT)
    find_package(OpenGL REQUIRED)
    find_package(SFML 2.6 COMPONENTS network system graphics REQUIRED)
else()
    find_package(SFML 2.6 COMPONENTS network system REQUIRED)
endif ()


include_directories(src)
include_directories(includes)
include_directories(interfaces)
include_directories(third_party/zlib)
include_directories(/usr/local/include)

file(GLOB_RECURSE CPP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HPP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

add_executable(server ${CPP} ${HPP})
target_link_libraries(server PRIVATE sfml-system sfml-network zlibstatic)

if(BUILD_CLIENT)
    add_executable(client ${CPP} ${HPP})

    target_compile_definitions(client PRIVATE BUILD_CLIENT)
    target_link_libraries(client PRIVATE sfml-graphics sfml-system sfml-network zlibstatic)
endif ()

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

print_all_variables()
