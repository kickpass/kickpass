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

include(FindPackageHandleStandardArgs)

find_path(SODIUM_INCLUDE_DIRS NAMES sodium.h)
find_library(SODIUM_LIBRARIES NAMES sodium)

find_package_handle_standard_args(SODIUM
	REQUIRED_VARS SODIUM_INCLUDE_DIRS SODIUM_LIBRARIES)

if (SODIUM_FOUND)
	file(STRINGS "${SODIUM_INCLUDE_DIRS}/sodium/version.h" _SODIUM_VERSION_H_CONTENT REGEX "#define SODIUM_VERSION_STRING ")
	STRING (REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" SODIUM_VERSION "${_SODIUM_VERSION_H_CONTENT}")

	if ("${Sodium_FIND_VERSION}" VERSION_GREATER "${SODIUM_VERSION}")
		message(SEND_ERROR "Invalid sodium version found ${SODIUM_VERSION} expecting at least ${Sodium_FIND_VERSION}")
	endif()
endif()
