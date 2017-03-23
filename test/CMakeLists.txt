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

include(Test)
include(CheckLibraryExists)

find_library(M_LIB m)
set(TEST_LIBS ${LIBS} ${M_LIB})
check_library_exists(c timer_create "" HAVE_TIMER_CREATE)
if (NOT HAVE_TIMER_CREATE)
	check_library_exists(rt timer_create "time.h" HAVE_TIMER_CREATE_IN_RT)
	if (HAVE_TIMER_CREATE_IN_RT)
		find_library(RT_LIB rt)
		set(TEST_LIBS ${TEST_LIBS} ${RT_LIB})
	else()
		message(FATAL_ERROR "can't find timer_create function")
	endif()
endif()
find_library(PTHREAD_LIB pthread)
set(TEST_LIBS ${TEST_LIBS} ${PTHREAD_LIB})

UNIT_TEST(NAME storage FILE storage.c LIBS libkickpass ${TEST_LIBS})
UNIT_TEST(NAME safe FILE safe.c LIBS libkickpass ${TEST_LIBS})
INTEGRATION_TEST(NAME init FILE init.py)
INTEGRATION_TEST(NAME create FILE create.py)
INTEGRATION_TEST(NAME edit FILE edit.py)
INTEGRATION_TEST(NAME list FILE list.py)
INTEGRATION_TEST(NAME cat FILE cat.py)
INTEGRATION_TEST(NAME open FILE open.py)
INTEGRATION_TEST(NAME delete FILE delete.py)
INTEGRATION_TEST(NAME rename FILE rename.py)
