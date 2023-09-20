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
include(GenerateExportHeader)

set(sources
    src/dds/core/Duration.cpp
    src/dds/core/Exception.cpp
    src/dds/core/Reference.cpp
    src/dds/core/Time.cpp
    src/dds/core/policy/CorePolicy.cpp
    src/dds/core/status/State.cpp
    src/dds/domain/discovery.cpp
    src/dds/domain/find.cpp
    src/dds/pub/pubdiscovery.cpp
    src/dds/sub/subdiscovery.cpp
    src/dds/sub/subfind.cpp
    src/dds/sub/status/DataState.cpp
    src/org/eclipse/cyclonedds/core/Mutex.cpp
    src/org/eclipse/cyclonedds/core/ObjectDelegate.cpp
    src/org/eclipse/cyclonedds/core/DDScObjectDelegate.cpp
    src/org/eclipse/cyclonedds/core/ObjectSet.cpp
    src/org/eclipse/cyclonedds/core/EntityDelegate.cpp
    src/org/eclipse/cyclonedds/core/ReportUtils.cpp
    src/org/eclipse/cyclonedds/core/ListenerDispatcher.cpp
    src/org/eclipse/cyclonedds/core/InstanceHandleDelegate.cpp
    src/org/eclipse/cyclonedds/core/EntitySet.cpp
    src/org/eclipse/cyclonedds/core/MiscUtils.cpp
    src/org/eclipse/cyclonedds/core/cdr/fragchain.cpp
    src/org/eclipse/cyclonedds/core/cdr/cdr_stream.cpp
    src/org/eclipse/cyclonedds/core/cdr/basic_cdr_ser.cpp
    src/org/eclipse/cyclonedds/core/cdr/entity_properties.cpp
    src/org/eclipse/cyclonedds/core/cdr/extended_cdr_v1_ser.cpp
    src/org/eclipse/cyclonedds/core/cdr/extended_cdr_v2_ser.cpp
    src/org/eclipse/cyclonedds/core/cond/ConditionDelegate.cpp
    src/org/eclipse/cyclonedds/core/cond/GuardConditionDelegate.cpp
    src/org/eclipse/cyclonedds/core/cond/StatusConditionDelegate.cpp
    src/org/eclipse/cyclonedds/core/cond/WaitSetDelegate.cpp
    src/org/eclipse/cyclonedds/core/policy/PolicyDelegate.cpp
    src/org/eclipse/cyclonedds/domain/Domain.cpp
    src/org/eclipse/cyclonedds/domain/DomainWrap.cpp
    src/org/eclipse/cyclonedds/domain/DomainParticipantDelegate.cpp
    src/org/eclipse/cyclonedds/domain/DomainParticipantRegistry.cpp
    src/org/eclipse/cyclonedds/domain/qos/DomainParticipantQosDelegate.cpp
    src/org/eclipse/cyclonedds/pub/AnyDataWriterDelegate.cpp
    src/org/eclipse/cyclonedds/pub/PublisherDelegate.cpp
    src/org/eclipse/cyclonedds/pub/qos/DataWriterQosDelegate.cpp
    src/org/eclipse/cyclonedds/pub/qos/PublisherQosDelegate.cpp
    src/org/eclipse/cyclonedds/sub/qos/DataReaderQosDelegate.cpp
    src/org/eclipse/cyclonedds/sub/AnyDataReaderDelegate.cpp
    src/org/eclipse/cyclonedds/sub/SubscriberDelegate.cpp
    src/org/eclipse/cyclonedds/sub/BuiltinSubscriberDelegate.cpp
    src/org/eclipse/cyclonedds/sub/QueryDelegate.cpp
    src/org/eclipse/cyclonedds/sub/SampleInfoImpl.cpp
    src/org/eclipse/cyclonedds/sub/cond/ReadConditionDelegate.cpp
    src/org/eclipse/cyclonedds/sub/cond/QueryConditionDelegate.cpp
    src/org/eclipse/cyclonedds/sub/qos/SubscriberQosDelegate.cpp
    src/org/eclipse/cyclonedds/topic/find.cpp
    src/org/eclipse/cyclonedds/topic/hash.cpp
    src/org/eclipse/cyclonedds/topic/AnyTopicDelegate.cpp
    src/org/eclipse/cyclonedds/topic/FilterDelegate.cpp
    src/org/eclipse/cyclonedds/topic/TopicDescriptionDelegate.cpp
    src/org/eclipse/cyclonedds/topic/qos/TopicQosDelegate.cpp)

if(BUILD_SHARED_LIBS OR NOT DEFINED BUILD_SHARED_LIBS)
  add_library(ddscxx SHARED ${sources})
else()
  add_library(ddscxx ${sources})
endif()

# SOVERSION should increase on incompatible ABI change
set_target_properties(ddscxx PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

if(ENABLE_LEGACY)
  target_compile_definitions(ddscxx PUBLIC "DDSCXX_USE_BOOST")
endif()

# Create a pseudo-target that other targets (i.e. examples, tests) can depend
# on and can also be provided as import-target by a package-file when building
# those targets outside the regular Cyclone build-tree (i.e. the installed tree)
add_library(${PROJECT_NAME}::ddscxx ALIAS ddscxx)

add_coverage(ddscxx)

set_property(TARGET ddscxx PROPERTY CXX_STANDARD ${cyclonedds_cpp_std_to_use})
target_link_libraries(ddscxx PUBLIC CycloneDDS::ddsc)
target_include_directories(
  ddscxx
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ddscxx>)

generate_export_header(
  ddscxx
  BASE_NAME OMG_DDS_API_DETAIL
  EXPORT_MACRO_NAME OMG_DDS_API_DETAIL
  EXPORT_FILE_NAME "include/dds/core/detail/export.hpp")

install(
  TARGETS ddscxx
  EXPORT "${PROJECT_NAME}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT lib
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib)

install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/dds"
            "${CMAKE_CURRENT_BINARY_DIR}/include/dds"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/org"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ddscxx"
  COMPONENT dev)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
