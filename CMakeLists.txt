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
cmake_minimum_required(VERSION 3.7)
project(CycloneDDS-CXX VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

# Conan
if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake" AND NOT CONAN_DEPENDENCIES)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  if(APPLE)
    # By default Conan strips all RPATHs (see conanbuildinfo.cmake), which
    # causes tests to fail as the executables cannot find the library target.
    # By setting KEEP_RPATHS, Conan does not set CMAKE_SKIP_RPATH and the
    # resulting binaries still have the RPATH information. This is fine because
    # CMake will strip the build RPATH information in the install step.
    #
    # NOTE:
    # Conan's default approach is to use the "imports" feature, which copies
    # all the dependencies into the bin directory. Of course, this doesn't work
    # quite that well for libraries generated in this Project (see Conan
    # documentation).
    #
    # See the links below for more information.
    # https://github.com/conan-io/conan/issues/337
    # https://docs.conan.io/en/latest/howtos/manage_shared_libraries/rpaths.html
    # https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
    conan_basic_setup(KEEP_RPATHS)
  else()
    conan_basic_setup()
  endif()
  conan_define_targets()
endif()

# By default don't treat warnings as errors, else anyone building it with a
# different compiler that just happens to generate a warning, as well as
# anyone adding or modifying something and making a small mistake would run
# into errors.  CI builds can be configured differently.
option(WERROR "Treat compiler warnings as errors." OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W3)
  if(WERROR)
    add_compile_options(/WX)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  add_compile_options(
    -Wall -Wextra -Wconversion -Wstrict-prototypes
    -Wunused -Winfinite-recursion -Wassign-enum -Wcomma -Wdocumentation
    -Wconditional-uninitialized -Wshadow)
  if(WERROR)
    add_compile_options(-Werror)
  endif()
  if(CMAKE_GENERATOR STREQUAL "Ninja")
    add_compile_options(-Xclang -fcolor-diagnostics)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
    -Wall -Wextra -Wconversion)
  if(WERROR)
    add_compile_options(-Werror)
  endif()
  if(CMAKE_GENERATOR STREQUAL "Ninja")
    add_compile_options(-fdiagnostics-color=always)
  endif()
endif()

if(CMAKE_GENERATOR STREQUAL "Xcode")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_BOOL_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_CONSTANT_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_FLOAT_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_INT_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_NON_LITERAL_NULL_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_IMPLICIT_SIGN_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_INFINITE_RECURSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_INITIALIZER_NOT_FULLY_BRACKETED YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_MISSING_PARENTHESES YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_NEWLINE YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_ASSIGN_ENUM YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SEMICOLON_BEFORE_METHOD_BODY YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SIGN_COMPARE YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_STRICT_PROTOTYPES YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_COMMA YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNINITIALIZED_AUTOS YES_AGGRESSIVE)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_LABEL YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_PARAMETER YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VALUE YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE YES)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_DOCUMENTATION_COMMENTS YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_PROTOTYPES YES)
endif()

if(CMAKE_VERSION VERSION_LESS 3.13)
  macro(add_link_options)
    link_libraries(${ARGV})
  endmacro()
endif()

# Make it easy to enable Clang and GCC analyzers
if(USE_SANITIZER)
  string(REGEX REPLACE " " "" USE_SANITIZER "${USE_SANITIZER}")
  string(REGEX REPLACE "[,;]+" ";" USE_SANITIZER "${USE_SANITIZER}")
  foreach(san ${USE_SANITIZER})
    if(san STREQUAL "address")
      add_compile_options("-fno-omit-frame-pointer")
      add_link_options("-fno-omit-frame-pointer")
    endif()
    if(san AND NOT san STREQUAL "none")
      message(STATUS "Enabling sanitizer: ${san}")
      add_compile_options("-fsanitize=${san}")
      add_link_options("-fsanitize=${san}")
    endif()
  endforeach()
endif()

# Build all executables and libraries into the top-level /bin and /lib folders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(BUILD_DOCS "Build documentation." OFF)
# By default building the testing tree is enabled by including CTest, but
# since it is not required to build the project, switch to off by default.
option(BUILD_TESTING "Build the testing tree." OFF)

include(GNUInstallDirs)
include(CTest)

add_subdirectory(src)
if(BUILD_DOCS)
  add_subdirectory(docs)
endif()
