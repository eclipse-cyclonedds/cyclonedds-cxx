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
  set(one_value_keywords TARGET DEFAULT_EXTENSIBILITY)
  set(multi_value_keywords FILES FEATURES INCLUDES WARNINGS)
  cmake_parse_arguments(
    IDLCXX "" "${one_value_keywords}" "${multi_value_keywords}" "" ${ARGN})

  find_package(CycloneDDS REQUIRED)

  # find idlcxx shared library
  if(CMAKE_CROSSCOMPILING)
    find_library(_idlcxx_shared_lib cycloneddsidlcxx NO_CMAKE_FIND_ROOT_PATH REQUIRED)

    if(_idlcxx_shared_lib)
      set(_idlcxx_depends "")
    else()
      message(FATAL_ERROR "Cannot find idlcxx shared library")
    endif()
  else()
    set(_idlcxx_shared_lib "$<TARGET_FILE:CycloneDDS-CXX::idlcxx>")
    set(_idlcxx_depends CycloneDDS-CXX::idlcxx)
  endif()

  idlc_generate_generic(TARGET ${IDLCXX_TARGET}
    BACKEND ${_idlcxx_shared_lib}
    FILES ${IDLCXX_FILES}
    FEATURES ${IDLCXX_FEATURES}
    INCLUDES ${IDLCXX_INCLUDES}
    WARNINGS ${IDLCXX_WARNINGS}
    DEFAULT_EXTENSIBILITY ${IDLCXX_DEFAULT_EXTENSIBILITY}
    SUFFIXES .hpp .cpp
    DEPENDS ${_idlcxx_depends}
  )
endfunction()
