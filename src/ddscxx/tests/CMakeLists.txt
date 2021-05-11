#
# Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
find_package(GTest REQUIRED)

idlcxx_generate(TARGET ddscxx_test_types FILES data/Space.idl data/HelloWorldData.idl)

configure_file(
  config_simple.xml.in config_simple.xml @ONLY)

set(sources
  EntityStatus.cpp
  Listener.cpp
  ListenerStress.cpp
  DomainParticipant.cpp
  Exception.cpp
  Conversions.cpp
  FindDataWriter.cpp
  FindDataReader.cpp
  FindTopic.cpp
  Topic.cpp
  Publisher.cpp
  Subscriber.cpp
  DataWriter.cpp
  DataReader.cpp
  DataReaderSelector.cpp
  DataReaderManipulatorSelector.cpp
  Duration.cpp
  Time.cpp
  Query.cpp
  WaitSet.cpp
  Qos.cpp
  Condition.cpp)

if(CMAKE_VERSION LESS "3.20")
    add_library(GTest::gtest ALIAS GTest::gtest)
    add_library(GTest::gtest_main ALIAS GTest::Main)
endif()

add_executable(ddscxx_tests ${sources})

# Disable the static analyzer in GCC to avoid crashing the GNU C++ compiler
# on Azure Pipelines
if(DEFINED ENV{SYSTEM_TEAMFOUNDATIONSERVERURI})
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND ANALYZER STREQUAL "on")
    target_compile_options(ddscxx_tests PRIVATE -fno-analyzer)
  endif()
endif()

set_property(TARGET ddscxx_tests PROPERTY CXX_STANDARD 17)
target_link_libraries(ddscxx_tests CycloneDDS-CXX::ddscxx ddscxx_test_types GTest::gtest GTest::gtest_main)
gtest_add_tests(TARGET ddscxx_tests SOURCES ${sources})
