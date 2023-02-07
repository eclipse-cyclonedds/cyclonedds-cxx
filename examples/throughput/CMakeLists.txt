#
# Copyright(c) 2019 to 2021 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
project(throughput LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.16)

if(NOT TARGET CycloneDDS-CXX::ddscxx)
  find_package(CycloneDDS-CXX REQUIRED)
endif()

# This is a convenience function, provided by the CycloneDDS package,
# that will supply a library target related the the given idl file.
# In short, it takes the idl file, generates the source files with
# the proper data types and compiles them into a library.
idlcxx_generate(TARGET Throughput_lib_cxx FILES Throughput.idl WARNINGS no-implicit-extensibility)

# Both executables have only one related source file.
add_executable(cxxThroughputPublisher publisher.cpp)
add_executable(cxxThroughputSubscriber subscriber.cpp)

# Both executables need to be linked to the idl data type library and
# the ddsc API library.
target_link_libraries(cxxThroughputPublisher Throughput_lib_cxx CycloneDDS-CXX::ddscxx)
target_link_libraries(cxxThroughputSubscriber Throughput_lib_cxx CycloneDDS-CXX::ddscxx)
