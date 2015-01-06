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

# Configure project
project(KickPass)

set (KickPass_VERSION_MAJOR 0)
set (KickPass_VERSION_MINOR 0)

configure_file(
	"${PROJECT_SOURCE_DIR}/src/kickpass_config.h.in"
	"${PROJECT_BINARY_DIR}/src/kickpass_config.h"
)

include_directories("${PROJECT_BINARY_DIR}/src/")

add_executable(kickpass
	src/main.c
	src/error.c)

# Configure dependencies
find_package(GPGME REQUIRED)
include_directories(${GPGME_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GPGME_LIBRARIES})

# Link all dependencies
target_link_libraries(kickpass ${LIBS})
