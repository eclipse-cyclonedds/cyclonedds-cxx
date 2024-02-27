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
project(helloworld LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

if(NOT TARGET CycloneDDS-CXX::ddscxx)
  find_package(CycloneDDS-CXX REQUIRED)
endif()

idlcxx_generate(TARGET helloworlddata FILES HelloWorldData.idl WARNINGS no-implicit-extensibility)

add_executable(ddscxxHelloworldPublisher publisher.cpp)
add_executable(ddscxxHelloworldSubscriber subscriber.cpp)

# Link both executables to idl data type library and ddscxx.
target_link_libraries(ddscxxHelloworldPublisher CycloneDDS-CXX::ddscxx helloworlddata)
target_link_libraries(ddscxxHelloworldSubscriber CycloneDDS-CXX::ddscxx helloworlddata)

# Disable the static analyzer in GCC to avoid crashing the GNU C++ compiler
# on Azure Pipelines
if(DEFINED ENV{SYSTEM_TEAMFOUNDATIONSERVERURI})
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND ANALYZER STREQUAL "on")
    target_compile_options(ddscxxHelloworldPublisher PRIVATE -fno-analyzer)
    target_compile_options(ddscxxHelloworldSubscriber PRIVATE -fno-analyzer)
  endif()
endif()

set_property(TARGET ddscxxHelloworldPublisher PROPERTY CXX_STANDARD ${cyclonedds_cpp_std_to_use})
set_property(TARGET ddscxxHelloworldSubscriber PROPERTY CXX_STANDARD ${cyclonedds_cpp_std_to_use})
