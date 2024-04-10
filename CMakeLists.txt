#
# Copyright(c) 2020 to 2022 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
cmake_minimum_required(VERSION 3.16)
project(CycloneDDS-CXX VERSION 0.11.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
set(CMAKE_CXX_STANDARD 11)

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
    -Wconditional-uninitialized -Wshadow -Wsign-conversion -Wpedantic)
  add_compile_options("$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-Wold-style-cast>")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0.0")
    #the dtor-name compiler option did not exist for older versions
  else()
    add_compile_options(-Wno-dtor-name) #suppressing these errors due to CLang adhering too close to the standard, which will be made more in the "spirit" of template classes starting C++23
  endif()
  if(WERROR)
    add_compile_options(-Werror)
  endif()
  if(CMAKE_GENERATOR STREQUAL "Ninja")
    add_compile_options(-Xclang -fcolor-diagnostics)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
    -Wall -Wextra -Wconversion -Wsign-conversion -Wpedantic)
  add_compile_options("$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-Wold-style-cast>")
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

if(CMAKE_CROSSCOMPILING)
  set(not_crosscompiling OFF)
else()
  set(not_crosscompiling ON)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

option(BUILD_DDSLIB "Build DDS lib" ON)

option(BUILD_IDLLIB "Build IDL preprocessor lib" ${not_crosscompiling})

if(NOT BUILD_DDSLIB AND NOT BUILD_IDLLIB)
  message(FATAL_ERROR "Nothing to build. At least one of the options BUILD_DDSLIB and/or BUILD_IDLLIB must be ON.")
endif()

# Make it easy to enable MSVC, Clang's/gcc's analyzers
set(ANALYZER "" CACHE STRING "Analyzer to enable on the build.")
if(ANALYZER)
  # GCC and Visual Studio offer builtin analyzers. Clang supports static
  # analysis through separate tools, e.g. Clang-Tidy, which can be used in
  # conjunction with other compilers too. Specifying -DANALYZER=on enables
  # the builtin analyzer for the compiler, enabling clang-tidy in case of
  # Clang. Specifying -DANALYZER=clang-tidy always enables clang-tidy.
  string(REPLACE " " "" ANALYZER "${ANALYZER}")
  string(TOLOWER "${ANALYZER}" ANALYZER)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND ANALYZER STREQUAL "on")
    set(ANALYZER "clang-tidy")
  endif()

  if(ANALYZER STREQUAL "clang-tidy")
    # Clang-Tidy is an extensible tool that offers more than static analysis.
    # https://clang.llvm.org/extra/clang-tidy/checks/list.html
    message(STATUS "Enabling analyzer: clang-tidy")
    set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.strcpy,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling")
    if(WERROR)
      set(CMAKE_C_CLANG_TIDY "${CMAKE_C_CLANG_TIDY};--warnings-as-errors=*")
    endif()
  elseif(ANALYZER STREQUAL "on")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
      if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "10")
        message(STATUS "Enabling analyzer: GCC")
        # -Wanalyzer-malloc-leak generates lots of false positives
        add_compile_options(-fanalyzer -Wno-analyzer-malloc-leak -Wno-analyzer-null-dereference)
      endif()
    elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
      message(STATUS "Enabling analyzer: MSVC")
      # Disable 6326, potential comparison of a constant with another constant.
      add_compile_options(/analyze:stacksize 524288 /wd6326)
    endif()
  endif()
endif()
set(SANITIZER "" CACHE STRING "Sanitizers to enable on the build.")
if(SANITIZER)
  string(REGEX REPLACE " " "" SANITIZER "${SANITIZER}")
  string(REGEX REPLACE "[,;]+" ";" SANITIZER "${SANITIZER}")
  foreach(san ${SANITIZER})
    if(san STREQUAL "address")
      add_compile_options("-fno-omit-frame-pointer")
      add_link_options("-fno-omit-frame-pointer")
    endif()
    if(san STREQUAL "undefined")
      add_compile_options("-fno-sanitize-recover=all")
    endif()
    if(san AND NOT san STREQUAL "none")
      message(STATUS "Enabling sanitizer: ${san}")
      add_compile_options("-fsanitize=${san}")
      add_link_options("-fsanitize=${san}")
    endif()
  endforeach()
endif()

find_package(codecov)
include(Codecov)

# Build all executables and libraries into the top-level /bin and /lib folders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# By default building the testing tree is enabled by including CTest, but
# since it is not required to build the project, switch to off by default.
option(BUILD_TESTING "Build the testing tree." OFF)
option(ENABLE_ICEORYX "Enable testing PSMX with Iceoryx plugin" OFF)

# Disable building the examples by default until the Idlpp-cxx has been
# deprecated.
option(BUILD_EXAMPLES "Build examples." OFF)

# Legacy c++11 compatibility - for projects which cant use
# the latest compiler features because they have to
# support some limited/legacy platforms (e.g. older QNX versions)
# this will add boost as a dependency
# c++11 seems like a good tradeoff between the old and the new world
option(ENABLE_LEGACY "Legacy c++11 compatibility." OFF)
set(cyclonedds_cpp_std_to_use 17)
if(ENABLE_LEGACY)
  set(cyclonedds_cpp_std_to_use 11)
  set(DDSCXX_USE_BOOST "1")
  find_package(Boost)
  include_directories(SYSTEM ${Boost_INCLUDE_DIR})
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CTest)

set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

# Generate <Package>Config.cmake
configure_package_config_file(
  "PackageConfig.cmake.in"
  "${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

# Generate <Package>Version.cmake
write_basic_package_version_file(
  "${PROJECT_NAME}Version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  COMPONENT dev)

# Generate <Package>Targets.cmake
install(
  EXPORT ${PROJECT_NAME}
  FILE "${PROJECT_NAME}Targets.cmake"
  NAMESPACE "${PROJECT_NAME}::"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  COMPONENT dev)

find_package(CycloneDDS REQUIRED)

get_target_property(cyclonedds_has_typelib CycloneDDS::ddsc TYPELIB_IS_AVAILABLE)
mark_as_advanced(cyclonedds_has_typelib)
option(ENABLE_TYPELIB "Enable Type Library support" ${cyclonedds_has_typelib})
if(ENABLE_TYPELIB)
  if (NOT cyclonedds_has_typelib)
    message(FATAL_ERROR "Cyclone DDS is not compiled with type library enabled")
  endif()
  message(STATUS "Compiling with type library support")
  set(DDSCXX_HAS_TYPELIB "1")
endif()

get_target_property(cyclonedds_has_topic_discovery CycloneDDS::ddsc TOPIC_DISCOVERY_IS_AVAILABLE)
mark_as_advanced(cyclonedds_has_topic_discovery)
option(ENABLE_TOPIC_DISCOVERY "Enable Topic Discovery support" ${cyclonedds_has_topic_discovery})
if(ENABLE_TOPIC_DISCOVERY)
  if (NOT cyclonedds_has_topic_discovery)
    message(FATAL_ERROR "Cyclone DDS is not compiled with topic discovery enabled")
  endif()
  if(NOT ENABLE_TYPELIB)
    message(FATAL_ERROR "ENABLE_TOPIC_DISCOVERY requires ENABLE_TYPELIB to be enabled")
  endif()
  message(STATUS "Compiling with topic discovery support")
  set(DDSCXX_HAS_TOPIC_DISCOVERY "1")
endif()

get_target_property(cyclonedds_has_qos_provider CycloneDDS::ddsc QOS_PROVIDER_IS_AVAILABLE)
mark_as_advanced(cyclonedds_has_qos_provider)
option(ENABLE_QOS_PROVIDER "Enable QoS Provider support" ${cyclonedds_has_qos_provider})
if(ENABLE_QOS_PROVIDER)
  if (NOT cyclonedds_has_qos_provider)
    message(FATAL_ERROR "Cyclone DDS is not compiled with qos provider enabled")
  endif()
  message(STATUS "Compiling with qos provider support")
  set(DDSCXX_HAS_QOS_PROVIDER "1")
endif()



configure_file(features.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/src/ddscxx/include/dds/features.hpp")

add_subdirectory(src)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

include(CMakeCPack.cmake)
