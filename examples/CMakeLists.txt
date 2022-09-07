#
# Copyright(c) 2020 to 2022 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
set(CMAKE_INSTALL_EXAMPLESDIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/examples")

# this is a work-around for cross-builds failing with:
#   CMake Error at examples/helloworld/CMakeLists.txt:19 (idlcxx_generate):
#   Unknown CMake command "idlcxx_generate".
include("${CMAKE_SOURCE_DIR}/src/idlcxx/Generate.cmake")

install(
  FILES helloworld/publisher.cpp
        helloworld/subscriber.cpp
        helloworld/HelloWorldData.idl
        helloworld/CMakeLists.txt
        helloworld/readme.rst
  DESTINATION "${CMAKE_INSTALL_EXAMPLESDIR}/helloworld"
  COMPONENT dev)

add_subdirectory(helloworld)

install(
  FILES throughput/publisher.cpp
        throughput/subscriber.cpp
        throughput/Throughput.idl
        throughput/CMakeLists.txt
        throughput/readme.rst
  DESTINATION "${CMAKE_INSTALL_EXAMPLESDIR}/throughput"
  COMPONENT dev)

add_subdirectory(throughput)

install(
  FILES roundtrip/ping.cpp
        roundtrip/ping.cpp
        roundtrip/roundtrip_common.hpp
        roundtrip/RoundTrip.idl
        roundtrip/CMakeLists.txt
        roundtrip/readme.rst
  DESTINATION "${CMAKE_INSTALL_EXAMPLESDIR}/throughput"
  COMPONENT dev)

add_subdirectory(roundtrip)
