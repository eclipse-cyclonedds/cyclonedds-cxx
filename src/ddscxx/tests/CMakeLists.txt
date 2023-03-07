#
# Copyright(c) 2006 to 2022 ZettaScale Technology and others
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

idlcxx_generate(TARGET ddscxx_test_types FILES
  data/Space.idl
  data/HelloWorldData.idl
  data/Serialization.idl
  data/CdrDataModels.idl
  data/CdrDataModels_pragma.idl
  data/EntityProperties.idl
  data/EntityProperties_pragma.idl
  data/ExtendedTypesModels.idl
  data/RegressionModels.idl
  data/RegressionModels_pragma.idl
  data/ExternalModels.idl
  data/TraitsModels.idl
  WARNINGS no-implicit-extensibility)

configure_file(
  config_simple.xml.in config_simple.xml @ONLY)

if(ENABLE_SHM)
  # Packages required packages for SHM tests
  find_package(iceoryx_posh REQUIRED)
  find_package(iceoryx_posh_testing REQUIRED)
endif()

set(sources
  Bounded.cpp
  EntityStatus.cpp
  Listener.cpp
  ListenerStress.cpp
  DomainParticipant.cpp
  Exception.cpp
  Conversions.cpp
  FindDataWriter.cpp
  FindDataReader.cpp
  Topic.cpp
  Publisher.cpp
  Serdata.cpp
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
  Condition.cpp
  Util.cpp
  CDRStreamer.cpp
  GeneratedEntities.cpp
  ExtendedTypes.cpp
  Regression.cpp
  External.cpp
  DataModels.cpp
  SampleInfo.cpp
  DeferredDestruction.cpp)

if (ENABLE_TYPE_DISCOVERY AND ENABLE_TOPIC_DISCOVERY)
  # Add topic/type discovery tests
  list(APPEND sources
    FindTopic.cpp
    TopicTypeDiscovery.cpp)
endif()

if(ENABLE_SHM)
  # Add shared memory tests
  list(APPEND sources SharedMemory.cpp)
endif()

add_executable(ddscxx_tests ${sources})

# Disable the static analyzer in GCC to avoid crashing the GNU C++ compiler
# on Azure Pipelines
if(DEFINED ENV{SYSTEM_TEAMFOUNDATIONSERVERURI})
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND ANALYZER STREQUAL "on")
    target_compile_options(ddscxx_tests PRIVATE -fno-analyzer)
  endif()
endif()

set_property(TARGET ddscxx_tests PROPERTY CXX_STANDARD ${cyclonedds_cpp_std_to_use})
target_link_libraries(
  ddscxx_tests PRIVATE
    CycloneDDS-CXX::ddscxx
    GTest::GTest
    GTest::Main
    ddscxx_test_types)

if(ENABLE_SHM)
  target_link_libraries(
    ddscxx_tests PRIVATE
      iceoryx_posh::iceoryx_posh_roudi
      iceoryx_posh_testing::iceoryx_posh_testing
      # iceoryx_posh_testing depends on GoogleMock but does not propagate the
      # the dependency. Workaround it by requiring the package here.
      GTest::GMock
      GTest::GMockMain)
endif()

target_link_libraries(ddscxx_tests ${TEST_LINK_LIBS})

gtest_add_tests(TARGET ddscxx_tests SOURCES ${sources} TEST_LIST tests)

# Ensure shared libraries are found
if(WIN32)
  set(sep ";")
  set(var "PATH")
elseif(APPLE)
  set(sep ":")
  set(var "DYLD_LIBRARY_PATH")
else()
  set(sep ":")
  set(var "LD_LIBRARY_PATH")
endif()

get_target_property(cyclonedds_lib CycloneDDS::ddsc LOCATION)
get_target_property(gtest_lib GTest::GTest IMPORTED_LOCATION)
get_target_property(gtest_main_lib GTest::Main IMPORTED_LOCATION)
if(ENABLE_SHM)
  get_target_property(gmock_lib GTest::GMock IMPORTED_LOCATION)
  get_target_property(gmock_main_lib GTest::GMockMain IMPORTED_LOCATION)
endif()

# Ignore false positives due to gtest not being compiled with asan
if(SANITIZER MATCHES "address")
  foreach(test ${tests})
    set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "ASAN_OPTIONS=detect_container_overflow=0")
  endforeach()
endif()

foreach(lib ${cyclonedds_lib} ${gtest_lib} ${gtest_main_lib} ${gmock_lib} ${gmock_main_lib})
  get_filename_component(libdir "${lib}" PATH)
  file(TO_NATIVE_PATH "${libdir}" libdir)

  foreach(test ${tests})
    get_property(envvars TEST ${test} PROPERTY ENVIRONMENT)
    list(LENGTH envvars n)
    set(add TRUE)
    if(n GREATER 0)
      math(EXPR n "${n} - 1")
      foreach(i RANGE 0 ${n})
        list(GET envvars ${i} envvar)
        if(envvar MATCHES "^${var}=")
          list(REMOVE_AT envvars ${i})
          set_property(TEST ${test} PROPERTY ENVIRONMENT "${envvars}")
          string(REGEX REPLACE "^${var}=" "" paths "${envvar}")
          string(REPLACE ";" "\\;" paths "${var}=${libdir}${sep}${paths}")
          set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "${paths}")
          set(add FALSE)
          break()
        endif()
      endforeach()
    endif()
    if(add)
      set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "${var}=${libdir}")
    endif()
  endforeach()
endforeach()
