#
# Copyright(c) 2021 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
function(IDLCXX_GENERATE)
  set(one_value_keywords TARGET)
  set(multi_value_keywords FILES FEATURES INCLUDES)
  cmake_parse_arguments(
    IDLCXX "" "${one_value_keywords}" "${multi_value_keywords}" "" ${ARGN})

  if(NOT IDLCXX_TARGET AND NOT IDLCXX_FILES)
    # assume deprecated invocation: TARGET FILE [FILE..]
    list(GET IDLCXX_UNPARSED_ARGUMENTS 0 IDLCXX_TARGET)
    list(REMOVE_AT IDLCXX_UNPARSED_ARGUMENTS 0)
    set(IDLCXX_FILES ${IDLCXX_UNPARSED_ARGUMENTS})
    if (IDLCXX_TARGET AND IDLCXX_FILES)
      message(WARNING " Deprecated use of idlcxx_generate. \n"
                      " Consider switching to keyword based invocation.")
    endif()
    # Java based compiler used to be case sensitive
    list(APPEND IDLCXX_FEATURES "case-sensitive")
  endif()

  if(NOT IDLCXX_TARGET)
    message(FATAL_ERROR "idlcxx_generate called without TARGET")
  elseif(NOT IDLCXX_FILES)
    message(FATAL_ERROR "idlcxx_generate called without FILES")
  endif()

  # remove duplicate features
  if(IDLCXX_FEATURES)
    list(REMOVE_DUPLICATES IDLCXX_FEATURES)
  endif()
  foreach(_feature ${IDLCXX_FEATURES})
    list(APPEND IDLCXX_ARGS "-f" ${_feature})
  endforeach()

  # add directories to include search list
  if(IDLCXX_INCLUDES)
    foreach(_dir ${IDLCXX_INCLUDES})
      list(APPEND IDLCXX_INCLUDE_DIRS "-I" ${_dir})
    endforeach()
  endif()

  set(_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(_target ${IDLCXX_TARGET})
  foreach(_file ${IDLCXX_FILES})
    get_filename_component(_path ${_file} ABSOLUTE)
    list(APPEND _files "${_path}")
  endforeach()

  foreach(_file ${_files})
    get_filename_component(_name ${_file} NAME_WE)
    set(_header "${_dir}/${_name}.hpp")
    list(APPEND _headers "${_header}")
    add_custom_command(
      OUTPUT   "${_header}"
      COMMAND  CycloneDDS::idlc
      ARGS     -l $<TARGET_FILE:CycloneDDS-CXX::idlcxx> ${IDLCXX_ARGS} ${IDLCXX_INCLUDE_DIRS} ${_file}
      DEPENDS  ${_files} CycloneDDS::idlc CycloneDDS-CXX::idlcxx)
  endforeach()

  add_custom_target("${_target}_generate" DEPENDS ${_headers})
  add_library(${_target} INTERFACE)
  target_sources(${_target} INTERFACE ${_headers})
  target_include_directories(${_target} INTERFACE "${_dir}")
  add_dependencies(${_target} "${_target}_generate")
endfunction()
