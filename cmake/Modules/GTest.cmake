#
# Copyright(c) 2006 to 2020 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
find_package(GTest REQUIRED)

set(GTEST_DIR "${CMAKE_CURRENT_LIST_DIR}/GTest")

function(parse_gtest_fixtures INPUT TEST_DISABLED TEST_TIMEOUT)
  if(INPUT MATCHES ".disabled${s}*=${s}*([tT][rR][uU][eE]|[0-9]+)")
    set(${TEST_DISABLED} "TRUE" PARENT_SCOPE)
  else()
    set(${TEST_DISABLED} "FALSE" PARENT_SCOPE)
  endif()

  if(INPUT MATCHES ".timeout${s}*=${s}*([0-9]+\\.?[0-9]*)")
    set(${TEST_TIMEOUT} "${CMAKE_MATCH_1}" PARENT_SCOPE)
  else()
    set(${TEST_TIMEOUT} "0" PARENT_SCOPE)
  endif()
endfunction()

# Parse a single source file, generate a header file with theory definitions
# (if applicable) and return suite and test definitions.
function(process_gtest_source_file SOURCE_FILE TESTS)
  set(s "[ \t\r\n]")
  set(w "[_a-zA-Z0-9]")
  set(ident_expr "(${s}*${w}+${s}*)")

  # Test fixture support to enable per-test timeouts and disabling.
  #
  # The following fixtures are supported:
  #  - disabled
  #  - timeout
  set(data_expr "(${s}*,${s}*\\.${w}+${s}*=[^,\\)]+)*")
  set(tests)

  file(READ "${SOURCE_FILE}" content)

  # G_Test
  set(test_expr "DDSCXX_(TEST|TEST_F|TEST_P)${s}*\\(${ident_expr},${ident_expr}${data_expr}\\)")
  string(REGEX MATCHALL "${test_expr}" matches "${content}")
  foreach(match ${matches})
    string(REGEX REPLACE "${test_expr}" "\\1" type "${match}")
    string(REGEX REPLACE "${test_expr}" "\\2" suite "${match}")
    string(REGEX REPLACE "${test_expr}" "\\3" test "${match}")
    # Remove leading and trailing whitespace
    string(STRIP "${type}" type)
    string(STRIP "${suite}" suite)
    string(STRIP "${test}" test)

    if(NOT test MATCHES "DISABLED_.*")
      # Extract fixtures that must be handled by CMake (.disabled and .timeout).
      parse_gtest_fixtures("${match}" disabled timeout)
      list(APPEND tests "${suite}:${test}:${type}:${disabled}:${timeout}")
    endif()
  endforeach()

  # Propagate tests extracted from the source file.
  if(tests)
	list(SORT tests)
  endif()
  set(${TESTS} ${tests} PARENT_SCOPE)
endfunction()

function(add_gtest_executable TARGET)
  # Retrieve location of shared libary, which is need to extend the PATH
  # environment variable on Microsoft Windows, so that the operating
  # system can locate the .dll that it was linked against.
  # On macOS, this mechanism is used to set the DYLD_LIBRARY_PATH.
  get_target_property(GTEST_LIBRARY_TYPE GTest::GTest TYPE)
  get_filename_component(GTEST_LIBRARY_DIR "${GTEST_LIBRARIES}" PATH)

  set(sources)

  foreach(source ${ARGN})
    if((EXISTS "${source}" OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}"))
      unset(tests)

      process_gtest_source_file("${source}" tests)

      foreach(test_item ${tests})
        string(REPLACE ":" ";" test_item ${test_item})
        list(GET test_item 4 timeout)
        list(GET test_item 3 disabled)
        list(GET test_item 2 type)
        list(GET test_item 1 test)
        list(GET test_item 0 suite)

        set(enable "true")
        if(disabled)
          set(enable "false")
        endif()
        if(NOT timeout)
          set(timeout 10)
        endif()

        set(ctest_name "GTest_${suite}_${test}")
        set(log_output_filename "${suite}-${test}")

        if (type STREQUAL "TEST_P")
          set(suite "*/${suite}")
          set(test "${test}/*")
        endif()
        add_test(
          NAME ${ctest_name}
          COMMAND ${TARGET} --gtest_output=xml:${log_output_filename}.xml --gtest_filter=${suite}.${test})

        set_property(TEST ${ctest_name} PROPERTY TIMEOUT ${timeout})
        set_property(TEST ${ctest_name} PROPERTY DISABLED ${disabled})
        if(APPLE)
          set_property(
            TEST ${ctest_name}
            PROPERTY ENVIRONMENT
              "DYLD_LIBRARY_PATH=${GTEST_LIBRARY_DIR}:$ENV{DYLD_LIBRARY_PATH}")
        elseif(WIN32 AND ${GTEST_LIBRARY_TYPE} STREQUAL "SHARED_LIBRARY")
          set_property(
            TEST ${ctest_name}
            PROPERTY ENVIRONMENT
              "PATH=${GTEST_LIBRARY_DIR};$ENV{PATH}")
        endif()
      endforeach()

      list(APPEND sources "${source}")
    endif()
  endforeach()

  add_executable(${TARGET} "${GTEST_DIR}/src/main.cpp" ${sources})
  target_link_libraries(${TARGET} PRIVATE GTest::GTest)
  target_include_directories(${TARGET} PRIVATE "${GTEST_DIR}/include")
endfunction()
