#
# Copyright(c) 2020 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#

add_library(
  idlcxx SHARED
    src/backendCpp11.c
    src/streamer_generator.c)

target_include_directories(
  idlcxx
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

target_link_libraries(idlcxx PUBLIC CycloneDDS::idl)

include(GenerateExportHeader)

generate_export_header(
  idlcxx
  BASE_NAME IDLCXX_EXPORT
  EXPORT_MACRO_NAME IDLCXX_EXPORT
  EXPORT_FILE_NAME "include/idlcxx/export.h")

install(
  TARGETS idlcxx
  EXPORT "${CMAKE_PROJECT_NAME}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT lib
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()