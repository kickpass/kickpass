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

include(CheckLibraryExists)

include(GetGitRevisionDescription)
git_describe(KickPass_VERSION "--always")
if (NOT KickPass_VERSION OR "$ENV{TRAVIS}")
	set(KickPass_VERSION_MAJOR 0)
	set(KickPass_VERSION_MINOR 2)
	set(KickPass_VERSION_PATCH 0)
else()
	message(STATUS "Kickpass git version: ${KickPass_VERSION}")
	string(REGEX REPLACE "v([0-9]+).*" "\\1"
		KickPass_VERSION_MAJOR ${KickPass_VERSION})
	string(REGEX REPLACE "v[0-9]+\\.([0-9]+).*" "\\1"
		KickPass_VERSION_MINOR ${KickPass_VERSION})
	string(REGEX REPLACE "v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1"
		KickPass_VERSION_PATCH ${KickPass_VERSION})
endif()
message(STATUS "Kickpass version: ${KickPass_VERSION_MAJOR}.${KickPass_VERSION_MINOR}.${KickPass_VERSION_PATCH}")

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
       set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

include_directories("${PROJECT_BINARY_DIR}/include/")
include_directories("${PROJECT_SOURCE_DIR}/include/")
include_directories("${PROJECT_SOURCE_DIR}/src/")

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
	src/command/cat.c
	src/command/rename.c
	src/command/agent.c
	src/command/open.c
)

# Configure dependencies
find_package(Sodium 1.0.0 REQUIRED)
message(STATUS "Libsodium version: ${SODIUM_VERSION}")
include_directories(${SODIUM_INCLUDE_DIRS})
set(LIB_LIBS ${LIB_LIBS} ${SODIUM_LIBRARIES})

check_library_exists(c readpassphrase "readpassphrase.h" HAS_READPASSPHRASE)
if (NOT HAS_READPASSPHRASE)
	find_package(BSD REQUIRED)
	include_directories(${BSD_INCLUDE_DIRS}/bsd)
	set(LIBS ${LIBS} ${BSD_LIBRARIES})
	set(LIB_LIBS ${LIB_LIBS} ${BSD_LIBRARIES})
	# Add _GNU_SOURCE for asprintf
	add_definitions(-DLIBBSD_OVERLAY -D_GNU_SOURCE)
endif()

check_library_exists(util imsg_init "imsg.h" HAS_IMSG)
if (HAS_IMSG)
	set(LIB_LIBS ${LIB_LIBS} util)
else()
	add_library(imsg STATIC
		${CMAKE_SOURCE_DIR}/compat/bsd/imsg-buffer.c
		${CMAKE_SOURCE_DIR}/compat/bsd/imsg.c
		${CMAKE_SOURCE_DIR}/compat/bsd/imsg.h
	)

	include_directories(${CMAKE_SOURCE_DIR}/compat/bsd/)
	set_target_properties(imsg PROPERTIES
		POSITION_INDEPENDENT_CODE ON
	)
	set(LIB_LIBS ${LIB_LIBS} imsg)
	set(PUBLIC_HEADERS ${PUBLIC_HEADERS} ${CMAKE_SOURCE_DIR}/compat/bsd/imsg.h)
endif()

find_package(Event2 REQUIRED)
include_directories(${EVENT2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${EVENT2_LIBRARIES})

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
set_target_properties(kickpass PROPERTIES C_STANDARD 99)

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
	lib/kpagent.c
)

file(GLOB PUBLIC_HEADERS
	${PROJECT_SOURCE_DIR}/include/*.h
	${PROJECT_BINARY_DIR}/include/*.h
	${PUBLIC_HEADERS}
)

set_target_properties(libkickpass PROPERTIES
	VERSION ${KickPass_VERSION_MAJOR}.${KickPass_VERSION_MINOR}.${KickPass_VERSION_PATCH}
	SOVERSION ${KickPass_VERSION_MAJOR}
	PUBLIC_HEADER "${PUBLIC_HEADERS}"
	OUTPUT_NAME kickpass
	POSITION_INDEPENDENT_CODE ON
)
set_target_properties(libkickpass PROPERTIES OUTPUT_NAME kickpass)
set_target_properties(libkickpass PROPERTIES C_STANDARD 99)

target_link_libraries(libkickpass ${LIB_LIBS})

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

install(TARGETS kickpass libkickpass
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include/kickpass
)
install(FILES manual/kickpass.1 DESTINATION share/man/man1/)
install(FILES extra/completion/zsh/_kickpass DESTINATION share/zsh/site-functions/)

# Tests
enable_testing()
add_subdirectory(test)

# Packaging
include(Package)
