#
# Copyright(c) 2026 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#

foreach(required_var IDLC IDLCXX_BACKEND IDL_FILE OUTPUT_DIR EXPECT_ERROR)
  if(NOT DEFINED ${required_var})
    message(FATAL_ERROR "${required_var} is required")
  endif()
endforeach()

file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

execute_process(
  COMMAND
    "${IDLC}"
    "-l${IDLCXX_BACKEND}"
    "-Wno-implicit-extensibility"
    "-o${OUTPUT_DIR}"
    "${IDL_FILE}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr)

set(output "${stdout}\n${stderr}")

if(result EQUAL 0)
  message(FATAL_ERROR
    "Expected ${IDL_FILE} to be rejected, but IDLC succeeded.\n"
    "Output:\n${output}")
endif()

if(NOT output MATCHES "${EXPECT_ERROR}")
  message(FATAL_ERROR
    "Expected rejection output for ${IDL_FILE} to match:\n"
    "  ${EXPECT_ERROR}\n"
    "Actual output:\n${output}")
endif()
