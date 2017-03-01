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
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CTEST_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)

# Configure project
project(KickPass C)

include(GetGitRevisionDescription)
git_describe(KickPass_VERSION "--always")
if (NOT KickPass_VERSION)
	set(KickPass_VERSION_MAJOR 0)
	set(KickPass_VERSION_MINOR 1)
	set(KickPass_VERSION_PATCH 1)
else()
	string(REGEX REPLACE "v([0-9]+).*" "\\1"
		KickPass_VERSION_MAJOR ${KickPass_VERSION})
	string(REGEX REPLACE "v[0-9]+\\.([0-9]+).*" "\\1"
		KickPass_VERSION_MINOR ${KickPass_VERSION})
	string(REGEX REPLACE "v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1"
		KickPass_VERSION_PATCH ${KickPass_VERSION})
endif()

include_directories("${PROJECT_BINARY_DIR}/include/")
include_directories("${PROJECT_SOURCE_DIR}/include/")
include_directories("${PROJECT_SOURCE_DIR}/src/")

# Configure portability
include(build/compat/bsd.cmake)

set(SRCS
	src/editor.c
	src/main.c
	src/prompt.c
	src/log.c
	# commands
	src/command/create.c
	src/command/delete.c
	src/command/edit.c
	src/command/init.c
	src/command/list.c
	src/command/open.c
	src/command/rename.c
)

# Configure dependencies
find_package(Sodium 1.0.0 REQUIRED)
message(STATUS "Libsodium version: ${SODIUM_VERSION}")
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
	"${PROJECT_SOURCE_DIR}/include/kickpass_config.h.in"
	"${PROJECT_BINARY_DIR}/include/kickpass_config.h"
)

add_executable(kickpass ${SRCS})

# Link all dependencies
target_link_libraries(kickpass ${LIBS})

# kickpass lib
add_library(libkickpass SHARED
	lib/config.c
	lib/error.c
	lib/kickpass.c
	lib/password.c
	lib/safe.c
	lib/storage.c
)

set_target_properties(libkickpass PROPERTIES
	VERSION ${KickPass_VERSION_MAJOR}.${KickPass_VERSION_MINOR}.${KickPass_VERSION_PATCH}
	SOVERSION ${KickPass_VERSION_MAJOR})
set_target_properties(libkickpass PROPERTIES OUTPUT_NAME kickpass)

target_link_libraries(kickpass libkickpass)

# Configure build
set(BUILD "Release" CACHE STRING "Selected build type")
if(NOT ("$ENV{BUILD}" STREQUAL ""))
	set(CMAKE_BUILD_TYPE $ENV{BUILD})
else()
	set(CMAKE_BUILD_TYPE ${BUILD})
endif()

message(STATUS "Build configuration (BUILD): ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DEFAULT_SOURCE -D_BSD_SOURCE")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -D_FORTIFY_SOURCE=2 -fstack-protector")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Werror")

# Configure install

install(TARGETS kickpass libkickpass RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
install(FILES manual/kickpass.1 DESTINATION share/man/man1/)
install(FILES extra/completion/zsh/_kickpass DESTINATION share/zsh/functions/Completion/Unix/)

# Tests
enable_testing()
add_subdirectory(test)

# Packaging
include(Package)
