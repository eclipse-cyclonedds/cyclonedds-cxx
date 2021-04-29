#
# Copyright(c) 2021 ADLINK Technology Limited and others
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
    src/types.c
    src/traits.c
    src/streamers.c
    src/generator.c)

target_link_libraries(idlcxx PUBLIC CycloneDDS::idl)

add_library(${PROJECT_NAME}::idlcxx ALIAS idlcxx)

add_coverage(idlcxx)

install(
  TARGETS idlcxx
  EXPORT "${CMAKE_PROJECT_NAME}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT idlcxx
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT idlcxx
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT idlcxx)

include(Generate.cmake)
