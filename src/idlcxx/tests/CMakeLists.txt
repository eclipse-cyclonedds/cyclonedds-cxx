#
# Copyright(c) 2006 to 2019 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
include(CUnit)

set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}")
set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}")

#the set of test case files which will be made into C code
set(test_generator_files
    ${source_dir}/generator_test_cases/bounded_sequence_impl.txt
    ${source_dir}/generator_test_cases/bounded_sequence_of_structs_impl.txt
    ${source_dir}/generator_test_cases/bounded_string_impl.txt
    ${source_dir}/generator_test_cases/cross_call_impl.txt
    ${source_dir}/generator_test_cases/keys_base_impl.txt
    ${source_dir}/generator_test_cases/keys_struct_explicit_impl.txt
    ${source_dir}/generator_test_cases/keys_struct_implicit_impl.txt
    ${source_dir}/generator_test_cases/keys_typedef_impl.txt
    ${source_dir}/generator_test_cases/keys_typedef_header.txt
    ${source_dir}/generator_test_cases/keys_union_implicit_impl.txt
    ${source_dir}/generator_test_cases/sequence_recursive_header.txt
    ${source_dir}/generator_test_cases/sequence_recursive_impl.txt
    ${source_dir}/generator_test_cases/struct_inheritance_impl.txt
    ${source_dir}/generator_test_cases/typedef_resolution_header.txt
    ${source_dir}/generator_test_cases/typedef_resolution_impl.txt)
set(generated_test_file ${binary_dir}/generator_test_cases.h)

#convert the test case files to a C header file
add_custom_command(
  OUTPUT ${generated_test_file}
  DEPENDS ${test_generator_files}
  COMMAND ${CMAKE_COMMAND} -DINPUT="${test_generator_files}" -DOUTPUT="${generated_test_file}" -P ${source_dir}/text_to_c.cmake)

#generate a library of the test cases
set(idlcxx_test_files "idlcxx_test_files")
add_library(${idlcxx_test_files} SHARED ${generated_test_file})
set_target_properties(${idlcxx_test_files} PROPERTIES LINKER_LANGUAGE C)

set(idlcxx_test_sources
    "cpp11Backend.c"
    "idl_ostream.c"
    "streamer_generator.c")

add_cunit_executable(cunit_idlcxx ${idlcxx_test_sources})
target_link_libraries(cunit_idlcxx PRIVATE idlcxx ${idlcxx_test_files})

#generated files (from function create_resources) are put here
target_include_directories(cunit_idlcxx PUBLIC ${binary_dir})

# Add location of CycloneDDS::idl to LD_LIBRARY_PATH, DYLD_LIBRARY_PATH, PATH
get_target_property(lib CycloneDDS::idl LOCATION)
get_filename_component(libdir "${lib}" PATH)
file(TO_NATIVE_PATH "${libdir}" libdir)

if(WIN32)
  set(sep ";")
  set(var "PATH")
elseif(APPLE)
  set(sep ":")
  set(var "DYLD_LIBRARY_PATH")
else()
  set(sep ":")
  set(var "LD_LIBRARY_PATH")
endif()

get_property(tests DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY TESTS)
foreach(test ${tests})
  get_property(envvars TEST ${test} PROPERTY ENVIRONMENT)
  list(LENGTH envvars n)
  foreach(i RANGE 0 ${n})
    list(GET envvars ${i} envvar)
    if(envvar MATCHES "^${var}=")
      list(REMOVE_AT envvars ${i})
      set_property(TEST ${test} PROPERTY ENVIRONMENT "${envvars}")
      string(REGEX REPLACE "^${var}=" "" paths "${envvar}")
      string(REPLACE ";" "\\;" paths "${var}=${libdir}${sep}${paths}")
      set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "${paths}")
      break()
    endif()
  endforeach()
endforeach()
