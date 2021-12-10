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
  set(multi_value_keywords FILES FEATURES)
  cmake_parse_arguments(
    IDLCXX "" "${one_value_keywords}" "${multi_value_keywords}" "" ${ARGN})

  if (CMAKE_CROSSCOMPILING)
    find_program(_idlc_executable idlc NO_CMAKE_FIND_ROOT_PATH REQUIRED)
    find_library(_idlcxx_shared_lib cycloneddsidlcxx NO_CMAKE_FIND_ROOT_PATH REQUIRED)

    if (_idlc_executable)
      set(_idlc_depends "")
    else()
      message(FATAL_ERROR "Cannot find idlc executable")
    endif()

    if (_idlcxx_shared_lib)
      set(_idlcxx_depends "")
    else()
      message(FATAL_ERROR "Cannot find idlcxx shared library")
    endif()
  else()
    set(_idlc_executable CycloneDDS::idlc)
    set(_idlc_depends CycloneDDS::idlc)
    set(_idlcxx_shared_lib "$<TARGET_FILE:CycloneDDS-CXX::idlcxx>")
    set(_idlcxx_depends CycloneDDS-CXX::idlcxx)
  endif()

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
      COMMAND  ${_idlc_executable}
      ARGS     -l ${_idlcxx_shared_lib} ${IDLCXX_ARGS} ${_file}
      DEPENDS  ${_files} ${_idlc_depends} ${_idlcxx_depends})
  endforeach()

  add_custom_target("${_target}_generate" DEPENDS ${_headers})
  add_library(${_target} INTERFACE)
  target_sources(${_target} INTERFACE ${_headers})
  target_include_directories(${_target} INTERFACE "${_dir}")
  add_dependencies(${_target} "${_target}_generate")
endfunction()
