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
project(helloworld_shm LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.5)

if(NOT TARGET CycloneDDS-CXX::ddscxx)
  find_package(CycloneDDS-CXX REQUIRED)
endif()

idlcxx_generate(TARGET helloworlddatashm FILES HelloWorldDataShm.idl)

add_executable(ddscxxHelloworldPublisherShm publisher_shm.cpp)
add_executable(ddscxxHelloworldSubscriberShm subscriber_shm.cpp)

# Link both executables to idl data type library and ddscxx.
target_link_libraries(ddscxxHelloworldPublisherShm CycloneDDS-CXX::ddscxx helloworlddatashm)
target_link_libraries(ddscxxHelloworldSubscriberShm CycloneDDS-CXX::ddscxx helloworlddatashm)

# Disable the static analyzer in GCC to avoid crashing the GNU C++ compiler
# on Azure Pipelines
if(DEFINED ENV{SYSTEM_TEAMFOUNDATIONSERVERURI})
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND ANALYZER STREQUAL "on")
    target_compile_options(ddscxxHelloworldPublisherShm PRIVATE -fno-analyzer)
    target_compile_options(ddscxxHelloworldSubscriberShm PRIVATE -fno-analyzer)
  endif()
endif()

set_property(TARGET ddscxxHelloworldPublisherShm PROPERTY CXX_STANDARD 17)
set_property(TARGET ddscxxHelloworldSubscriberShm PROPERTY CXX_STANDARD 17)
