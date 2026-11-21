#
# Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
cmake_minimum_required(VERSION 3.5)
find_package(Idlpp-cxx REQUIRED)
find_package(CycloneDDS REQUIRED)

if (NOT TARGET CycloneDDS_CXX_API::ddscxx)
    # Find the CycloneDDS_CXX_API package. If it is not in a default location, try
    # finding it relative to the example where it most likely resides.
    find_package(CycloneDDS_CXX_API REQUIRED PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
endif()

# This is a convenience function, provided by the CycloneDDS_CXX_API package,
# that will supply a library target related the the given idl file.
# In short, it takes the idl file, generates the source files with
# the proper data types and compiles them into a library.
idl_ddscxx_generate(ddscxxATCDataModel_lib "ATCDataModel.idl")

# Both executables have only one related source file.
add_executable(pubATC publisher.cpp)
add_executable(subATC subscriber.cpp)

# Both executables need to be linked to the idl data type library and
# the ddsc API library.
target_link_libraries(pubATC ddscxxATCDataModel_lib CycloneDDS_CXX_API::ddscxx)
target_link_libraries(subATC ddscxxATCDataModel_lib CycloneDDS_CXX_API::ddscxx)

set_property(TARGET pubATC PROPERTY CXX_STANDARD 11)
set_property(TARGET subATC PROPERTY CXX_STANDARD 11)
