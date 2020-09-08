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

set(idlcxx_test_sources
    "cpp11Backend.c"
    "idl_ostream.c"
    "streamer_generator.c")

add_cunit_executable(cunit_idlcxx ${idlcxx_test_sources})
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
