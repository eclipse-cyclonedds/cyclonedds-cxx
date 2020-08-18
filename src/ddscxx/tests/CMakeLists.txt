#
# Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
include(GTest)

find_package(Idlpp-cxx REQUIRED PATHS )
if(NOT Idlpp-cxx_FOUND)
  message(WARNING "Could not find Idlpp-cxx - please provide it through CMAKE_PREFIX_PATH")
  return()
endif()

# Setup environment for config-tests
set(gtest_ddscxx_config_simple_file "${CMAKE_CURRENT_BINARY_DIR}/config_simple.xml")
set(gtest_ddscxx_config_simple_uri "file://${gtest_ddscxx_config_simple_file}")
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/config_simple.xml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/config_simple.xml"
        @ONLY)
configure_file("config_env.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/config_env.hpp")

set(ddscxx_test_sources
  "ExampleTests.cpp"
  "EntityStatus.cpp"
  "ListenerSetup.cpp"
  "ListenerTrigger.cpp"
  "ListenerStress.cpp"
  "DomainParticipant.cpp"
  "Exception.cpp"
  "Conversions.cpp"
  "FindDataWriter.cpp"
  "FindDataReader.cpp"
  "FindTopic.cpp"
  "Topic.cpp"
  "Publisher.cpp"
  "Subscriber.cpp"
  "DataWriter.cpp"
  "DataReader.cpp"
  "DataReaderSelector.cpp"
  "DataReaderManipulatorSelector.cpp"
  "Duration.cpp"
  "Time.cpp"
  "Query.cpp"
  "WaitSet.cpp"
  "Condition.cpp"
  "Qos.cpp"
)

idl_ddscxx_generate(TestsIdls data/Space.idl data/HelloWorldData.idl)

add_gtest_executable(gtest_ddscxx ${ddscxx_test_sources})
set_property(TARGET gtest_ddscxx PROPERTY CXX_STANDARD 11)
target_link_libraries(gtest_ddscxx PRIVATE TestsIdls ddscxx CycloneDDS::ddsc)
target_include_directories(
  gtest_ddscxx PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/include/>")
