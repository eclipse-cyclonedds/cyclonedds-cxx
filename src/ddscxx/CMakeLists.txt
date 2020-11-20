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

target_include_directories(
  ddscxx
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ddscxx>)

set_property(TARGET ddscxx PROPERTY CXX_STANDARD 11)
target_link_libraries(ddscxx PUBLIC CycloneDDS::ddsc)

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

# Preprocess headers to make the documentation more readable. The steps
# involved have been converted from a Python script into CMake logic. This is
# better solved in another way, but it will have to do for now.
if(BUILD_DOCS)
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

  # Template class names that must be replaced.
  set(templates
    "TBuiltinTopic" "TDomainParticipant" "TEntity" "TInstanceHandle"
    "TQosProvider" "TCondition" "TGuardCondtion" "TStatusCondition"
    "TWaitSet" "TCorePolicy" "TQosPolicy" "TStatus" "TDomainParticipant"
    "TCoherentSet" "TPublisher" "TSuspendedPublication" "TCoherentAccess"
    "TDataReader" "TGenerationCount" "TQuery" "TRank" "TSample" "TSubscriber"
    "TReadCondition" "TFilter" "TGuardCondition" "THolder" "TDHolder"
    "TAnyDataReader" "TAnyTopic" "TAnyDataWriter"
    # XTypes
    "TAnnotation" "TCollectionTypes" "TDynamic" "TMember" "TStruct" "TType"
    "TCollectionType" "TExtensibilityAnnotation" "TidAnnotation"
    "TKeyAnnotation" "TPrimitiveType" "TSequenceType" "TStringType"
    "TUnionForwardDeclaration" "TVerbatimAnnotation" "TBitBoundAnnotation"
    "TBitsetAnnotation" "TMapType" "TMustUnderstandAnnotation"
    "TNestedAnnotation" "TIdAnnotation" "TUnionForwardDeclaration"
    # QoS
    "TUserData" "TGroupData" "TTopic" "TTransportPriority" "TLifespan"
    "TDeadline" "TLatencyBudget" "TTimeBasedFilter" "TPartition" "TOwnership"
    "TWriterDataLifecycle" "TReaderDataLifecycle" "TDurability" "TPresentation"
    "TReliability" "TDestinationOrder" "THistory" "TResourceLimits"
    "TLiveliness" "TDurabilityService" "TShare" "TProductData"
    "TSubscriptionKey" "TDataRepresentation" "TRequestedDeadlineMissedStatus"
    "TInconsistentTopicStatus" "TOffered" "TRequested"
    # TBuiltinStuff
    "TSubscription" "TPublication" "TParticipant" "TTopicBuiltinTopicData"
    "TCM" "TBuiltinTopicTypes" "TBytesTopicType" "TKeyedBytesTopicType"
    "TKeyedStringTopicType" "TStringTopicType"
    # Streams
    "TStreamDataReader" "TStreamDataWriter" "TCorePolicy" "TStreamSample"
    "TStreamFlush")

  # Sequences with DELAGATE that must be removed.
  set(delegates
    "template <typename DELEGATE>" "<typename DELEGATE>" "<D>" "<DELEGATE>"
    "< DELEGATE >" "template <typename D>" "< DELEGATE<T> >"
    ", template <typename Q> class DELEGATE" ", DELEGATE")

  foreach(input ${headers})
    set(output "${input}")
    foreach(fromto ${rename})
      if(fromto MATCHES "^${input}:")
        string(REPLACE "${input}:" "" output "${fromto}")
        break()
      endif()
    endforeach()

    file(READ "${input_dir}/${input}" content)

    # Rename the struct to the typedef and remove the typedef from the sources
    # for readability.
    set(safe_enum "typedef +dds::core::safe_enum<([a-zA-Z0-9_]+)> +([a-zA-Z0-9]+)")
    string(REGEX MATCHALL "${safe_enum};" typedefs "${content}")
    foreach(typedef ${typedefs})
      string(REGEX REPLACE "${safe_enum}" "\\1" struct_name "${typedef}")
      string(REGEX REPLACE "${safe_enum}" "\\2" typedef_name "${typedef}")
      string(REPLACE "${struct_name}" "${typedef_name}" content "${content}")
      string(REGEX REPLACE "${safe_enum};" "" content "${content}")
    endforeach()

    foreach(search ${templates})
      string(SUBSTRING ${search} 1 -1 replace)
      string(REPLACE ${search} ${replace} content "${content}")
    endforeach()
    foreach(search ${delegates})
      string(REPLACE ${search} "" content "${content}")
    endforeach()
    file(WRITE "${output_dir}/${output}" "${content}")
    list(APPEND input_files "${output_dir}/${output}")
  endforeach()

  find_package(Doxygen REQUIRED)
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
