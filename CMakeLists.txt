#
# Copyright (c) 2015 Paul Fariello <paul@fariello.eu>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# Configure cmake
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CTEST_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)

# Configure project
project(KickPass C)

set(KickPass_VERSION_MAJOR 0)
set(KickPass_VERSION_MINOR 1)
set(KickPass_VERSION_PATCH 0)

include_directories("${PROJECT_BINARY_DIR}/src/")
include_directories("${PROJECT_SOURCE_DIR}/include/")

# Configure portability
include(build/compat/bsd.cmake)

set(SRCS
	src/main.c
	src/kickpass.c
	src/storage.c
	src/editor.c
	src/safe.c
	src/password.c
	# commands
	src/command/init.c
	src/command/create.c
	src/command/open.c
	src/command/edit.c
)

# Configure dependencies
find_package(Sodium 1.0.2 REQUIRED)
include_directories(${SODIUM_INCLUDE_DIRS})
set(LIBS ${LIBS} ${SODIUM_LIBRARIES})

find_package(X11)
if (X11_FOUND)
	include_directories(${X11_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${X11_LIBRARIES})
	set(SRCS ${SRCS} src/command/copy.c)
	set(HAS_X11 true)
else()
	message(WARNING "X11 not found. Skipping copy command.")
endif()

# Sources
configure_file(
	"${PROJECT_SOURCE_DIR}/src/kickpass_config.h.in"
	"${PROJECT_BINARY_DIR}/src/kickpass_config.h"
)

add_executable(kickpass ${SRCS})

# Link all dependencies
target_link_libraries(kickpass ${LIBS})

# Configure build
set(BUILD "Release" CACHE STRING "Selected build type")
if(NOT ("$ENV{BUILD}" STREQUAL ""))
	set(CMAKE_BUILD_TYPE $ENV{BUILD})
else()
	set(CMAKE_BUILD_TYPE ${BUILD})
endif()

message(STATUS "Build configuration (BUILD): ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Werror")

# Tests
enable_testing()
add_subdirectory(test)

