cmake_minimum_required(VERSION 3.27)

project(snake-app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_FIND_FRAMEWORK NEVER)
set(SFML_STATIC_LIBRARIES ON)
set(BUILD_SHARED_LIBS OFF)

add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)
set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-deprecated-declarations"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# ===============================
# Platform-specific setup
# ===============================
if (APPLE)
    list(PREPEND CMAKE_PREFIX_PATH
            "/opt/homebrew"
            "/usr/local"
    )
else()
    # Windows / MinGW
    list(PREPEND CMAKE_PREFIX_PATH "C:/mingw64")

    # MSYS2 MinGW64 deps: OpenSSL + Boost
    list(PREPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")

    set(OPENSSL_ROOT_DIR "C:/msys64/mingw64" CACHE PATH "" FORCE)
    set(BOOST_ROOT       "C:/msys64/mingw64" CACHE PATH "" FORCE)

    add_compile_options(-Wa,-mbig-obj)
endif()

set(BUILD_SHARED_LIBS OFF)

# ===============================
# snake-shared options
# ===============================
set(VSHARED_ENABLE_UTILS            ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_LOGGING          ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_WEBSOCKET        ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_UDP              ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_SERVICE_LOADER   ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_COROUTINE        ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_CRYPT            ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_LEGACY_GAME      ON CACHE BOOL "" FORCE)

# ===============================
# Dependencies
# ===============================
find_package(OpenGL REQUIRED)

find_package(SFML 2.6
        COMPONENTS system network graphics
        CONFIG REQUIRED
)

include_directories(
        src
        /usr/local/include
)

add_subdirectory(snake-shared)

file(GLOB_RECURSE SNAKE_SERVER_SOURCES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

add_executable(snake-app
        ${SNAKE_SERVER_SOURCES}
)

target_link_libraries(snake-app
        PRIVATE
        snake-shared::all
        sfml-graphics
        sfml-system
        sfml-network
)

target_compile_features(snake-app PUBLIC cxx_std_23)
