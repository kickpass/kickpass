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

include(FindPythonModule)

find_package(Check)
find_program(VALGRIND_COMMAND valgrind)
find_package(PythonInterp)

if (VALGRIND_COMMAND)
	set(VALGRIND_OPTIONS "-q --tool=memcheck --show-reachable=yes --leak-check=full --gen-suppressions=all --track-origins=yes")
	set(VALGRIND_OPTIONS "${VALGRIND_OPTIONS} --suppressions=${CTEST_MODULE_PATH}/test-functional.sup")
endif()

if (CHECK_FOUND)
	macro(UNIT_TEST)
		set(oneValueArgs NAME FILE)
		set(multiValueArgs LIBS)
		cmake_parse_arguments(UNIT_TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
		set(TEST_NAME test-unit-${UNIT_TEST_NAME})

		add_executable(${TEST_NAME} ${UNIT_TEST_FILE})
		target_link_libraries(${TEST_NAME} ${CHECK_LIBRARIES})
		target_link_libraries(${TEST_NAME} ${UNIT_TEST_LIBS})
		set_property(TARGET ${TEST_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${CHECK_INCLUDE_DIRS})
		add_test(NAME ${TEST_NAME}
			COMMAND ${TEST_NAME})
	endmacro(UNIT_TEST)
else()
	message(WARNING "Check not found. Skipping unit tests !")
	macro(UNIT_TEST)
	endmacro(UNIT_TEST)
endif()

if (PYTHON_EXECUTABLE)
	find_python_module(pexpect)
	if (PY_PEXPECT)
		macro(INTEGRATION_TEST)
			set(oneValueArgs NAME FILE)
			cmake_parse_arguments(INTEGRATION_TEST "" "${oneValueArgs}" "" ${ARGN})
			set(TEST_NAME test-functional-${INTEGRATION_TEST_NAME})

			add_test(NAME ${TEST_NAME}
				COMMAND ${CTEST_MODULE_PATH}/TestFunctionalDriver.sh $<TARGET_FILE:kickpass> "${TEST_NAME}" "${CMAKE_CURRENT_SOURCE_DIR}/${INTEGRATION_TEST_FILE}")

			set(TEST_ENV "")
			list(APPEND TEST_ENV "PYTHON=${PYTHON_EXECUTABLE}")
			list(APPEND TEST_ENV "EDITOR_PATH=${CTEST_MODULE_PATH}")
			if (VALGRIND_COMMAND)
				list(APPEND TEST_ENV "VALGRIND_COMMAND=${VALGRIND_COMMAND}")
				list(APPEND TEST_ENV "VALGRIND_OPTIONS=${VALGRIND_OPTIONS}")
			endif()
			set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "${TEST_ENV}")
		endmacro(INTEGRATION_TEST)
	else()
		message(WARNING "Python pexpect module not found. Skipping functional tests !")
		macro(INTEGRATION_TEST)
		endmacro(INTEGRATION_TEST)
	endif()
else()
	message(WARNING "Python not found. Skipping functional tests !")
	macro(INTEGRATION_TEST)
	endmacro(INTEGRATION_TEST)
endif()
