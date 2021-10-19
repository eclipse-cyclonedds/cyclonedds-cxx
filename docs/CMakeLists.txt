#
# Copyright(c) 2020 ADLINK Technology Limited and others
# Copyright(c) 2021 Apex.AI Inc. All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE
        ${CMAKE_CURRENT_SOURCE_DIR}/../README.md
  DESTINATION share/doc/cyclonedds-cxx)

if(BUILD_DOCS)
  find_package(Sphinx REQUIRED breathe exhale)

  sphinx_add_docs(
    docs
    BREATHE_PROJECTS ddscxx_api_docs
    BUILDER html
    SOURCE_DIRECTORY manual)

  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}/manual"
    COMPONENT dev)
endif()
