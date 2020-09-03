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
project(helloworld LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.5)

if (NOT TARGET CycloneDDS-CXX::ddscxx)
  find_package(CycloneDDS-CXX REQUIRED)
endif()

set(sources HelloWorldData.cpp datatopic.cpp)
set(headers HelloWorldData.h HelloWorldData_DCPS.hpp datatopic.hpp)

add_library(ddscxxHelloWorldData_lib OBJECT ${sources} ${headers})
target_link_libraries(ddscxxHelloWorldData_lib PUBLIC  CycloneDDS-CXX::ddscxx)

add_executable(ddscxxHelloworldPublisher publisher.cpp)
add_executable(ddscxxHelloworldSubscriber subscriber.cpp)

# Link both executables to idl data type library and ddscxx.
target_link_libraries(ddscxxHelloworldPublisher ddscxxHelloWorldData_lib CycloneDDS-CXX::ddscxx)
target_link_libraries(ddscxxHelloworldSubscriber ddscxxHelloWorldData_lib CycloneDDS-CXX::ddscxx)

set_property(TARGET ddscxxHelloworldPublisher PROPERTY CXX_STANDARD 11)
set_property(TARGET ddscxxHelloworldSubscriber PROPERTY CXX_STANDARD 11)
