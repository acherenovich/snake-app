cmake_minimum_required(VERSION 3.27)
project(snake-app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT APPLE)
    set(CMAKE_FIND_FRAMEWORK NEVER)
endif()
set(BUILD_SHARED_LIBS OFF)

set(SFML_STATIC_LIBRARIES ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_NO_BOOST_CMAKE ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)
add_compile_definitions(SFML_STATIC BOOST_ASIO_NO_DEPRECATED)

set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-deprecated-declarations"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# ===============================
# Platform-specific setup (MSYS2 MinGW64)
# ===============================
if (WIN32)
    if (DEFINED ENV{MINGW_PREFIX})
        file(TO_CMAKE_PATH "$ENV{MINGW_PREFIX}" MSYS2_MINGW64)
    else()
        set(MSYS2_MINGW64 "C:/msys64/mingw64")
    endif()

    list(PREPEND CMAKE_PREFIX_PATH "${MSYS2_MINGW64}")

    include_directories("${MSYS2_MINGW64}/include")
    link_directories("${MSYS2_MINGW64}/lib")

    set(OPENSSL_ROOT_DIR "${MSYS2_MINGW64}" CACHE PATH "" FORCE)
    set(BOOST_ROOT       "${MSYS2_MINGW64}" CACHE PATH "" FORCE)
    set(OPENSSL_INCLUDE_DIR "${MSYS2_MINGW64}/include" CACHE PATH "" FORCE)
    set(OPENSSL_SSL_LIBRARY "${MSYS2_MINGW64}/lib/libssl.a" CACHE FILEPATH "" FORCE)
    set(OPENSSL_CRYPTO_LIBRARY "${MSYS2_MINGW64}/lib/libcrypto.a" CACHE FILEPATH "" FORCE)

    add_compile_options(-Wa,-mbig-obj)

    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_LINK_SEARCH_START_STATIC ON)
    set(CMAKE_LINK_SEARCH_END_STATIC   ON)

    add_link_options(-static -static-libgcc -static-libstdc++)

    add_link_options(
            -Wl,-Bstatic
            -Wl,--whole-archive -lwinpthread -Wl,--no-whole-archive
            -Wl,-Bdynamic
    )

endif()

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

find_package(SFML 3 REQUIRED COMPONENTS Graphics System Network Window)

find_package(Boost REQUIRED COMPONENTS json container)
find_package(OpenSSL REQUIRED)

include_directories(src)

# ===============================
# Subprojects
# ===============================
add_subdirectory(snake-shared)

file(GLOB_RECURSE SNAKE_APP_SOURCES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

add_executable(snake-app ${SNAKE_APP_SOURCES})
target_compile_features(snake-app PUBLIC cxx_std_23)

if (WIN32)
    set(MSYS_LIB "${MSYS2_MINGW64}/lib")

    set(PNG_A        "${MSYS_LIB}/libpng16.a")
    set(Z_A          "${MSYS_LIB}/libz.a")
    set(BZ2_A        "${MSYS_LIB}/libbz2.a")
    set(BROTLIDEC_A  "${MSYS_LIB}/libbrotlidec.a")
    set(BROTLICOM_A  "${MSYS_LIB}/libbrotlicommon.a")
    set(HARFBUZZ_A   "${MSYS_LIB}/libharfbuzz.a")
    set(FREETYPE_A   "${MSYS_LIB}/libfreetype.a")

    foreach(LIBFILE IN LISTS PNG_A Z_A BZ2_A BROTLIDEC_A BROTLICOM_A HARFBUZZ_A FREETYPE_A)
        if (NOT EXISTS "${LIBFILE}")
            message(FATAL_ERROR
                    "НЕ НАЙДЕН СТАТИЧЕСКИЙ АРХИВ: ${LIBFILE}\n"
                    "Значит MSYS2 поставил только DLL/импорт-либу. Нужен пакет с *.a.")
        endif()
    endforeach()
endif()

target_link_libraries(snake-app
        PRIVATE
        snake-shared::all

        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Network

        OpenGL::GL

        "${FREETYPE_A}"
        "${HARFBUZZ_A}"
        "${PNG_A}"
        "${Z_A}"
        "${BZ2_A}"
        "${BROTLIDEC_A}"
        "${BROTLICOM_A}"

        # иногда нужно добить z в конце при -static
        "${Z_A}"
)

if (WIN32)
    target_link_libraries(snake-app PRIVATE
            "${MSYS_LIB}/libgraphite2.a"
    )

    target_link_libraries(snake-app PRIVATE
            rpcrt4
            dwrite
    )

    target_link_options(snake-app PRIVATE -Wl,-Bstatic)
endif()
