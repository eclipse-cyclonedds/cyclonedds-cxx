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
set(input "${INPUT}")
set(output "${OUTPUT}")

if(NOT input)
  message(FATAL_ERROR "INPUT is not set")
elseif(NOT EXISTS ${input})
  message(FATAL_ERROR "INPUT '${input}' does not exist")
endif()

if(NOT output)
  message(FATAL_ERROR "OUTPUT is not set")
endif()

# Turn filename into identifier
get_filename_component(name "${input}" NAME)
# Convert non-alphanumeric characters to underscores
string(MAKE_C_IDENTIFIER "${name}" name)
# Read file and convert to a hexadecimal representation
file(READ "${input}" hexdata HEX)
#add terminating NULL to the end of the string
string(CONCAT hexdata ${hexdata} "00")
# Mimic xxd output
# Replace windows line endings (CR+LF) with linux line endings (LF)
string(REPLACE "0d0a" "0a" hexdata ${hexdata})
string(REGEX REPLACE "(........................)" "  \\1\n" hexdata ${hexdata})
string(REGEX REPLACE "\n([^\n]+)$" "\n  \\1" hexdata ${hexdata})
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " hexdata ${hexdata})
string(REGEX REPLACE ",[ \t]+$" "" hexdata ${hexdata})

file(WRITE  "${output}" "const unsigned char ${name}[] = {\n")
file(APPEND "${output}" ${hexdata})
file(APPEND "${output}" "\n};\n")
file(APPEND "${output}" "const unsigned ${name}_size = sizeof(${name});\n")
