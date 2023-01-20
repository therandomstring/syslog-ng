#############################################################################
# Copyright (c) 2017 Balabit
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As an additional exemption you are allowed to compile & link against the
# OpenSSL libraries as published by the OpenSSL project. See the file
# COPYING for details.
#
#############################################################################


include(CMakeParseArguments)

if (BUILD_TESTING)
  include(CheckPIESupported)
  check_pie_supported()
endif()

function (add_unit_test)

  if (NOT BUILD_TESTING)
    return()
  endif()

  cmake_parse_arguments(ADD_UNIT_TEST "CRITERION;LIBTEST" "TARGET" "SOURCES;DEPENDS;INCLUDES" ${ARGN})

  if (NOT ADD_UNIT_TEST_SOURCES)
    set(ADD_UNIT_TEST_SOURCES "${ADD_UNIT_TEST_TARGET}.c")
  endif()

  add_executable(${ADD_UNIT_TEST_TARGET} ${ADD_UNIT_TEST_SOURCES})
  target_compile_definitions(${ADD_UNIT_TEST_TARGET} PRIVATE TOP_SRCDIR="${PROJECT_SOURCE_DIR}")
  target_link_libraries(${ADD_UNIT_TEST_TARGET} ${ADD_UNIT_TEST_DEPENDS} syslog-ng)
  target_include_directories(${ADD_UNIT_TEST_TARGET} PUBLIC ${ADD_UNIT_TEST_INCLUDES})
  if (NOT APPLE)
    set_property(TARGET ${ADD_UNIT_TEST_TARGET} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--no-as-needed")
  endif()

  if (${ADD_UNIT_TEST_CRITERION})
    target_link_libraries(${ADD_UNIT_TEST_TARGET} ${CRITERION_LIBRARIES})
    target_include_directories(${ADD_UNIT_TEST_TARGET} PUBLIC ${CRITERION_INCLUDE_DIRS})
    set_property(TARGET ${ADD_UNIT_TEST_TARGET} PROPERTY POSITION_INDEPENDENT_CODE FALSE)

  endif()

  if (${ADD_UNIT_TEST_LIBTEST})
    target_link_libraries(${ADD_UNIT_TEST_TARGET} libtest)
  endif()

  add_test (${ADD_UNIT_TEST_TARGET} ${ADD_UNIT_TEST_TARGET})
  add_dependencies(check ${ADD_UNIT_TEST_TARGET})
  set_tests_properties(${ADD_UNIT_TEST_TARGET} PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_odr_violation=0;CRITERION_TEST_PATTERN=!(*/*performance*)")
  set_tests_properties(${ADD_UNIT_TEST_TARGET} PROPERTIES FAIL_REGULAR_EXPRESSION "ERROR: (LeakSanitizer|AddressSanitizer)")
endfunction ()

macro (add_test_subdirectory SUBDIR)
  if (BUILD_TESTING)
    add_subdirectory(${SUBDIR})
  endif()
endmacro()

function (add_fuzz_test)

  #module check
  if(NOT ENABLE_FUZZING)
      return()
  endif()
  if(BUILD_TESTING)
    message(WARNING "\
!!!BUILD_TESTING defined!!!
It is strongly recommended not to run unit tests while fuzzing.\
Fuzz test should ideally run for a longer duration, which may conflict with your unit test environment.\
Also, fuzzing need not be done as often as unit testing.\
    ")
  endif()

  cmake_parse_arguments(ADD_FUZZ_TEST "EXPERIMENTAL;MEMORY_FUNCTIONS;NO_SANITIZE" "TARGET;CORPUS_DIR;TIMEOUT;TESTCASE_TIMEOUT" "SRC;LIBS;EXEC_PARMS" ${ARGN})

  #pre-check
  if (ADD_FUZZ_TEST_EXPERIMENTAL AND ADD_FUZZ_TEST_NO_SANITIZE)
    message(FATAL_ERROR "EXPERIMENTAL and NO_SANITIZE should not be used together.")
  endif ()
  if (ADD_FUZZ_TEST_MEMORY_FUNCTIONS AND ADD_FUZZ_TEST_NO_SANITIZE)
    message(FATAL_ERROR "MEMORY_FUNCTIONS and NO_SANITIZE should not be used together.")
  endif ()

  if(NOT ADD_FUZZ_TEST_SRC)
    message(NOTICE "No source file was provided for fuzzing target ${ADD_FUZZ_TEST_TARGET}. Trying targets/${ADD_FUZZ_TEST_TARGET}.c")
    set(ADD_FUZZ_TEST_SRC targets/${ADD_FUZZ_TEST_TARGET}.c)
  endif()
  if(NOT ADD_FUZZ_TEST_CORPUS_DIR)
    message(NOTICE "No corpus directory was provided for fuzzing target ${ADD_FUZZ_TEST_TARGET}. Trying corpora/")
    set(ADD_FUZZ_TEST_CORPUS_DIR ${PROJECT_SOURCE_DIR}/tests/fuzzing/tests/${ADD_FUZZ_TEST_TARGET}/corpora)
  else()
    set(ADD_FUZZ_TEST_CORPUS_DIR ${PROJECT_SOURCE_DIR}/tests/fuzzing/tests/${ADD_FUZZ_TEST_TARGET}/${ADD_FUZZ_TEST_CORPUS_DIR})
  endif()

  add_executable(${ADD_FUZZ_TEST_TARGET} ${ADD_FUZZ_TEST_SRC})
  target_include_directories(${ADD_FUZZ_TEST_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/tests/fuzzing/lib)
  target_include_directories(${ADD_FUZZ_TEST_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/modules)
  #configure flags
  if(ADD_FUZZ_TEST_MEMORY_FUNCTIONS)
    add_compile_definitions(SYSLOG_NG_FUZZING_MALLOC=1)
  else()
    add_compile_definitions(SYSLOG_NG_FUZZING_MALLOC=0)
  endif()
  if(ADD_FUZZ_TEST_EXPERIMENTAL)
    set(ADD_FUZZ_TEST_EXEC_PARMS ${ADD_FUZZ_TEST_EXEC_PARMS} -print_final_stats=1 -detect_leaks=1)
    set_target_properties(${ADD_FUZZ_TEST_TARGET} PROPERTIES COMPILE_FLAGS "-o1 -fsanitize=\"fuzzer,memory,signed-integer-overflow,null,alignment\" -fno-omit-frame-pointer")
    set_target_properties(${ADD_FUZZ_TEST_TARGET} PROPERTIES LINK_FLAGS "-o1 -fsanitize=\"fuzzer,memory,signed-integer-overflow,null,alignment\" -fno-omit-frame-pointer")
  elseif(ADD_FUZZ_TEST_NO_SANITIZE)
    message(NOTICE "Fuzing without sanitizer")
    set(ADD_FUZZ_TEST_EXEC_PARMS ${ADD_FUZZ_TEST_EXEC_PARMS} -detect_leaks=0)
    set_target_properties(${ADD_FUZZ_TEST_TARGET} PROPERTIES COMPILE_FLAGS "-o1 -fsanitize=\"fuzzer\" -fno-omit-frame-pointer")
    set_target_properties(${ADD_FUZZ_TEST_TARGET} PROPERTIES LINK_FLAGS "-o1 -fsanitize=\"fuzzer\" -fno-omit-frame-pointer")
    if(ADD_FUZZ_TEST_MEMORY_FUNCTIONS)
      message(NOTICE "The option `MEMORY_FUNCTIONS` can't is only designed to work with sanitizers. It won't be used.")
      set(${ADD_FUZZ_TEST_MEMORY_FUNCTIONS} 0)
    endif()
  else()
    set(ADD_FUZZ_TEST_EXEC_PARMS ${ADD_FUZZ_TEST_EXEC_PARMS} -detect_leaks=0)
    set_target_properties(${ADD_FUZZ_TEST_TARGET} PROPERTIES COMPILE_FLAGS "-o1 -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer")
    set_target_properties(${ADD_FUZZ_TEST_TARGET} PROPERTIES LINK_FLAGS "-o1 -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer")
  endif()
  #configure libraries
  target_link_libraries(${ADD_FUZZ_TEST_TARGET} PRIVATE syslog-ng ${ADD_FUZZ_TEST_LIBS})
  #set timeout
  if(ADD_FUZZ_TEST_TIMEOUT LESS 20)
    message(FATAL_ERROR "Test timeout set too short.")
  endif()
  if(NOT ADD_FUZZ_TEST_TIMEOUT)
    set(ADD_FUZZ_TEST_TIMEOUT 1500)
  endif()
  math(EXPR ADD_FUZZ_TEST_FUZZER_TOTAL_TIMEOUT ${ADD_FUZZ_TEST_TIMEOUT}-10)
  if(NOT ADD_FUZZ_TEST_TESTCASE_TIMEOUT)
    set(ADD_FUZZ_TEST_TESTCASE_TIMEOUT 60)
  endif()
  if(ADD_FUZZ_TEST_TESTCASE_TIMEOUT GREATER ADD_FUZZ_TEST_FUZZER_TOTAL_TIMEOUT)
    message(WARNING "Total timeout is not significantly longer than the per-testcase timeout duration.\nThis might lead to problems.")
  endif()

  add_test(fuzz_${ADD_FUZZ_TEST_TARGET} ${ADD_FUZZ_TEST_TARGET} -max_total_time=${ADD_FUZZ_TEST_FUZZER_TOTAL_TIMEOUT} -timeout=${ADD_FUZZ_TEST_TESTCASE_TIMEOUT} ${ADD_FUZZ_TEST_EXEC_PARMS} ${ADD_FUZZ_TEST_CORPUS_DIR} )
  set_tests_properties(fuzz_${ADD_FUZZ_TEST_TARGET} PROPERTIES TIMEOUT ${ADD_FUZZ_TEST_TIMEOUT})
  if (ADD_FUZZ_TEST_MEMORY_FUNCTIONS)
    message(NOTICE "Building fuzz target with non-standard memory management functions.")
    set_tests_properties(fuzz_${ADD_FUZZ_TEST_TARGET} PROPERTIES ENVIRONMENT "LD_PRELOAD=${PROJECT_BINARY_DIR}/tests/fuzzing/lib/libfuzzing_intercept_memory_functions.so")
    if (ADD_FUZZ_TEST_EXPERIMENTAL)
      set_property(TEST fuzz_${ADD_FUZZ_TEST_TARGET} PROPERTY ENVIRONMENT "MSAN_OPTIONS=allocator_may_return_null=1")
    else()
      set_property(TEST fuzz_${ADD_FUZZ_TEST_TARGET} PROPERTY ENVIRONMENT "ASAN_OPTIONS=allocator_may_return_null=1")
    endif ()
  endif ()
endfunction()
