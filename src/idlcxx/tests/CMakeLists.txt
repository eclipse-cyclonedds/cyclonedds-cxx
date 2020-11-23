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
set(generator_dir "${source_dir}/generator")

set(sources
  cpp11Backend.c
  idl_ostream.c
  streamer_generator.c)

add_cunit_executable(cunit_idlcxx ${sources})

set(xxd_input
  ${generator_dir}/bounded_sequence_impl.cpp.in
  ${generator_dir}/bounded_sequence_of_structs_impl.cpp.in
  ${generator_dir}/bounded_string_impl.cpp.in
  ${generator_dir}/cross_call_impl.cpp.in
  ${generator_dir}/keys_base_impl.cpp.in
  ${generator_dir}/keys_struct_explicit_impl.cpp.in
  ${generator_dir}/keys_struct_implicit_impl.cpp.in
  ${generator_dir}/keys_typedef_impl.cpp.in
  ${generator_dir}/keys_typedef_header.cpp.in
  ${generator_dir}/keys_union_implicit_impl.cpp.in
  ${generator_dir}/sequence_recursive_header.cpp.in
  ${generator_dir}/sequence_recursive_impl.cpp.in
  ${generator_dir}/struct_inheritance_impl.cpp.in
  ${generator_dir}/typedef_resolution_header.cpp.in
  ${generator_dir}/typedef_resolution_impl.cpp.in)

foreach(input ${xxd_input})
  get_filename_component(name ${input} NAME_WE)
  set(output "${binary_dir}/${name}.c")
  add_custom_command(
    OUTPUT ${output}
    DEPENDS ${input}
    COMMAND ${CMAKE_COMMAND}
      ARGS -DINPUT="${input}" -DOUTPUT="${output}" -P ${source_dir}/xxd.cmake)
  list(APPEND xxd_output ${output})
endforeach()

target_sources(cunit_idlcxx PRIVATE ${xxd_output})
target_link_libraries(cunit_idlcxx PRIVATE idlcxx)

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
