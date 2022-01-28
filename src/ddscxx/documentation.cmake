#
# Copyright(c) 2022 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
if(NOT INPUT_FILE)
  message(FATAL_ERROR "INPUT_FILE not specified")
elseif(NOT EXISTS "${INPUT_FILE}")
  message(FATAL_ERROR "INPUT_FILE (${INPUT_FILE}) does not exist")
endif()

if(NOT OUTPUT_DIRECTORY)
  message(FATAL_ERROR "OUTPUT_DIRECTORY not specified")
elseif(NOT EXISTS "${OUTPUT_DIRECTORY}")
  message(FATAL_ERROR "OUTPUT_DIRECTORY (${OUTPUT_DIRECTORY}) does not exist")
endif()

if(NOT OUTPUT_FILE)
  message(FATAL_ERROR "OUTPUT_FILE not specified")
#elseif(EXISTS "${OUTPUT_FILE}")
#  # ensure no existing file is overwritten (including input)
#  message(FATAL_ERROR "OUTPUT_FILE (${OUTPUT_FILE}) already exists")
endif()

# normalize paths
file(TO_CMAKE_PATH "${INPUT_FILE}" input_file)
file(TO_CMAKE_PATH "${OUTPUT_DIRECTORY}" output_directory)
file(TO_CMAKE_PATH "${OUTPUT_FILE}" output_file)

if(IS_ABSOLUTE "${output_file}")
  string(FIND "${output_file}" "${output_directory}" position)
  if(NOT position EQUAL 0)
    message(FATAL_ERROR "OUTPUT_FILE (${OUTPUT_FILE}) is not relative to OUTPUT_DIRECTORY (${OUTPUT_DIRECTORY})")
  endif()
  file(RELATIVE_PATH output_file "${output_directory}" "${output_file}")
endif()

file(READ "${input_file}" content)

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

foreach(search ${templates})
  string(SUBSTRING ${search} 1 -1 replace)
  string(REPLACE ${search} ${replace} content "${content}")
endforeach()

# Sequences with DELAGATE that must be removed.
set(delegates
  "template <typename DELEGATE>" "<typename DELEGATE>" "<D>" "<DELEGATE>"
  "< DELEGATE >" "template <typename D>" "< DELEGATE<T> >"
  ", template <typename Q> class DELEGATE" ", DELEGATE")

foreach(search ${delegates})
  string(REPLACE ${search} "" content "${content}")
endforeach()

# ensure output directory exists
get_filename_component(relative_path "${output_file}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}/${relative_path}")
file(WRITE "${output_directory}/${output_file}" "${content}")
