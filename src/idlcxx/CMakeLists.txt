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
include(GenerateExportHeader)

add_library(
  idlcxx SHARED
    src/backend.c
    src/backendCpp11Utils.c
    src/backendCpp11Type.c
    src/backendCpp11Trait.c
    src/streamer_generator.c
    src/idl_ostream.c
    src/generator.c)

target_include_directories(
  idlcxx
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

target_link_libraries(idlcxx PUBLIC CycloneDDS::idl)

generate_export_header(
  idlcxx
  BASE_NAME IDLCXX_EXPORT
  EXPORT_MACRO_NAME IDLCXX_EXPORT
  EXPORT_FILE_NAME "include/idlcxx/export.h")

add_library(${PROJECT_NAME}::idlcxx ALIAS idlcxx)

install(
  TARGETS idlcxx
  EXPORT "${CMAKE_PROJECT_NAME}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT idlcxx
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT idlcxx
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT idlcxx)

install(
  FILES Generate.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/idlcxx"
  COMPONENT idlcxx)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

include(Generate.cmake)
