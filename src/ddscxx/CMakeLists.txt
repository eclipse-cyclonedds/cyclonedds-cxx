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

# Create a pseudo-target that other targets (i.e. examples, tests) can depend
# on and can also be provided as import-target by a package-file when building
# those targets outside the regular Cyclone build-tree (i.e. the installed tree)
add_library(${CMAKE_PROJECT_NAME}::ddscxx ALIAS ddscxx)

add_coverage(ddscxx)

set_property(TARGET ddscxx PROPERTY CXX_STANDARD ${cyclonedds_cpp_std_to_use})

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Microsoft does not specify the __cplusplus macro by default
  # https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus
  target_compile_options(ddscxx PUBLIC "/Zc:__cplusplus")
endif()

target_link_libraries(ddscxx PUBLIC CycloneDDS::ddsc)
target_include_directories(
  ddscxx
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ddscxx>)

if(ENABLE_SHM)
  # TODO: Ideally the C++ language binding shouldn't depend directly on
  #       Iceoryx, but should get it from Cyclone DDS
  target_link_libraries(ddscxx PUBLIC iceoryx_binding_c::iceoryx_binding_c)
endif()

generate_export_header(
  ddscxx
  BASE_NAME OMG_DDS_API_DETAIL
  EXPORT_MACRO_NAME OMG_DDS_API_DETAIL
  EXPORT_FILE_NAME "include/dds/core/detail/export.hpp")

install(
  TARGETS ddscxx
  EXPORT "${CMAKE_PROJECT_NAME}"
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

if(BUILD_DOCS)
  find_package(Doxygen REQUIRED)

  # Preprocess headers to make the documentation more readable.
  set(script "${PROJECT_SOURCE_DIR}/src/ddscxx/documentation.cmake")
  set(input_dir "${CMAKE_CURRENT_SOURCE_DIR}/include")
  set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
  file(GLOB_RECURSE headers RELATIVE "${input_dir}" CONFIGURE_DEPENDS "include/*.hpp")
  foreach(header ${headers})
    get_filename_component(directory ${header} DIRECTORY)
    get_filename_component(filename ${header} NAME)
    # Templates are documented. e.g. dds/sub/TQuery.hpp contains the code
    # and documentation for a the dds::sub::Query class which is a typedef of
    # dds::sub::detail::Query, which itself is a typedef to the delegate that
    # implements the template. This must be hidden from the user, so replace
    # the original header by that of the template and replace the typenames in
    # the source file.
    if(("${filename}" MATCHES "^T") AND
       (NOT "${filename}" MATCHES "^(Topic|Time|Type)"))
      string(SUBSTRING "${filename}" 1 -1 non_template)
      list(APPEND rename "${directory}/${filename}:${directory}/${non_template}")
      if(EXISTS "${input_dir}/${directory}/${non_template}")
        list(APPEND remove "${directory}/${non_template}")
      endif()
    # Use file located in detail for Qoses (filename ends with Qos.hpp).
    # Except TEntityQos.hpp, EntityQos container is not a real Qos and must
    # not be treated as such.
    elseif((filename MATCHES "Qos.hpp$") AND
           (EXISTS "${input_dir}/${directory}/detail/${filename}"))
      list(APPEND remove "${directory}/${filename}")
      list(APPEND rename "${directory}/detail/${filename}:${header}")
    endif()
  endforeach()

  foreach(header ${remove})
    list(REMOVE_ITEM headers ${header})
  endforeach()

  set(input_files)
  foreach(input ${headers})
    set(output "${input}")
    foreach(fromto ${rename})
      if(fromto MATCHES "^${input}:")
        string(REPLACE "${input}:" "" output "${fromto}")
        break()
      endif()
    endforeach()
    add_custom_command(
      OUTPUT "${output_dir}/${output}"
      COMMAND "${CMAKE_COMMAND}" -DINPUT_FILE="${input_dir}/${input}"
                                 -DOUTPUT_DIRECTORY="${output_dir}"
                                 -DOUTPUT_FILE="${output}"
                                 -P "${script}"
      DEPENDS "${input_dir}/${input}" "${script}")
    list(APPEND input_files "${output_dir}/${output}")
  endforeach()

  set(DOXYGEN_PROJECT_NAME "Eclipse CycloneDDS C++ API Reference Guide")
  set(DOXYGEN_STRIP_FROM_PATH "${CMAKE_CURRENT_BINARY_DIR}/include")
  set(DOXYGEN_EXCLUDE_PATTERNS
    "*/include/dds/*/detail/*"
    "*/include/org/eclipse/*"
    "*/dds/core/xtypes/*")
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_XML YES)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_PREDEFINED
    "DOXYGEN_FOR_ISOCPP=1"
    "OMG_DDS_API=")
  doxygen_add_docs(ddscxx_api_docs ${input_files})
endif()
