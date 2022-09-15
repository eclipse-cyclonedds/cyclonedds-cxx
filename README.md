# C++ binding for Eclipse Cyclone DDS

An implementation of the [ISO/IEC C++ PSM][1], or simply put, a C++ binding
for [Eclipse Cyclone DDS][2]. Cyclone DDS is developed completely in the open
as an Eclipse IoT project (see [eclipse-cyclone-dds][3]) with a growing list
of [adopters][4] (if you're one of them, please add your [logo][5]). It is a
tier-1 middleware for the Robot Operating System [ROS 2][6].

[1]: https://www.omg.org/spec/DDS-PSM-Cxx/
[2]: https://github.com/eclipse-cyclonedds/cyclonedds/
[3]: https://projects.eclipse.org/projects/iot.cyclonedds
[4]: https://iot.eclipse.org/adopters/?#iot.cyclonedds
[5]: https://github.com/EclipseFdn/iot.eclipse.org/issues/new?template=adopter_request.md
[6]: https://index.ros.org/doc/ros2/

[![Build Status](https://dev.azure.com/eclipse-cyclonedds/cyclonedds-cxx/_apis/build/status/Pull%20requests?branchName=master)](https://dev.azure.com/eclipse-cyclonedds/cyclonedds-cxx/_build/latest?definitionId=4&branchName=master)
[![Coverity Status](https://scan.coverity.com/projects/21579/badge.svg)](https://scan.coverity.com/projects/eclipse-cyclonedds-cyclonedds-cxx)
[![Codecov](https://codecov.io/gh/eclipse-cyclonedds/cyclonedds-cxx/branch/master/graphs/badge.svg?branch=master)](https://codecov.io/github/eclipse-cyclonedds/cyclonedds-cxx?branch=master)
[![License](https://img.shields.io/badge/License-EPL%202.0-blue)](https://choosealicense.com/licenses/epl-2.0/)
[![License](https://img.shields.io/badge/License-EDL%201.0-blue)](https://choosealicense.com/licenses/edl-1.0/)

# Getting Started

## Building the Eclipse Cyclone DDS C++ binding

In order to build the C++ binding for Cyclone DDS you need a Linux, Mac or
Windows 10 machine (or, with some caveats, a \*BSD, OpenIndiana one) with the
following installed on your host:

 * C and C++ compilers (most commonly GCC on Linux, Visual Studio on Windows,
   Xcode on macOS);
 * [Git](https://git-scm.com/) version control system;
 * [CMake](https://cmake.org/download/), version 3.16 or later;
 * [Eclipse Cyclone DDS](https://github.com/eclipse-cyclonedds/cyclonedds/)

*Eclipse Cyclone DDS* has dependencies of its own, most notably Bison. To
build and install it, please consult the build instructions. Ensure the
project is installed into a location convenient for you by specifying
`CMAKE_INSTALL_PREFIX`.

To obtain the C++ binding for Cyclone DDS, do

    $ git clone https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git
    $ cd cyclonedds-cxx
    $ mkdir build

Depending on whether you want to develop applications using the C++ binding
for Cyclone DDS or contribute to it you can follow different procedures.

### Build configuration

There are some configuration options specified using CMake defines in addition to the standard options like `CMAKE_BUILD_TYPE`:
* `-DBUILD_DDSLIB=OFF`: to disable DDS lib build, useful in crosscompiling scenarios where you only need the generator and use the DDS lib from another build.
* `-DBUILD_IDLLIB=OFF`: to disable IDL preprocessor lib build
* `-DBUILD_DOCS=ON`: to build the documentation
* `-DBUILD_TESTING=ON`: to build the testing tree
* `-DBUILD_EXAMPLES=ON`: to build examples
* `-DENABLE_LEGACY=YES`: to enable legacy c++11 mode, adds boost as dependency (otherwise it uses c++17)
* `-DENABLE_SHM=YES`: to enable shared memory support
* `-DENABLE_TYPE_DISCOVERY=YES`: to enable type discovery support
* `-DENABLE_TOPIC_DISCOVERY=YES`: to enable topic discovery support 
* `-DENABLE_COVERAGE=YES`: to enable coverage build

### For application developers

To build and install the required libraries needed to develop your own
applications using the C++ binding for Cyclone DDS requires a few simple
steps. There are some small differences between Linux and macOS on the one
hand, and Windows on the other. For Linux or macOS:

    $ cd build
    $ cmake -DCMAKE_INSTALL_PREFIX=<install-location> \
            -DCMAKE_PREFIX_PATH="<cyclonedds-install-location>" \
            ..
    $ cmake --build .

and for Windows:

    $ cd build
    $ cmake -G "<generator-name>" \
            -DCMAKE_INSTALL_PREFIX=<install-location> \
            -DCMAKE_PREFIX_PATH="<cyclonedds-install-location>" \
            ..
    $ cmake --build .

where you should replace `<install-location>` by the directory under which you
would like to install the C++ binding for Cyclone DDS and `<generator-name>`
by one of the ways CMake [generators][6] offer for generating build files. For
example, "Visual Studio 15 2017 Win64" would target a 64-bit build using
Visual Studio 2017.

To install it after a successful build, do:

    $ cmake --build . --target install

Which will copy everything to:

 * `<install-location>/lib`
 * `<install-location>/bin`
 * `<install-location>/include/ddsc`
 * `<install-location>/share/CycloneDDS-CXX`

Depending on the installation location you may need administrator privileges.

At this point you are ready to use Eclipse Cyclone DDS in your own projects.

Note that the default build type is a release build with debug information
included (RelWithDebInfo), which is generally the most convenient type of
build to use from applications because of a good mix between performance and
still being able to debug things. If you'd rather have a Debug or pure Release
build, set `CMAKE_BUILD_TYPE` accordingly.

[6]: https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html

### Contributing to Eclipse Cyclone DDS

We very much welcome all contributions to the project, whether that is
questions, examples, bug fixes, enhancements or improvements to the
documentation, or anything else really. When considering contributing code,
it might be good to know that build configurations for Travis CI and AppVeyor
are present in the repository and that there is a test suite using CTest and
Google Test that can be built locally if desired. To build it, set the cmake
variable `BUILD_TESTING` to on when configuring, e.g.:

    $ cd build
    $ cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON ..
    $ cmake --build .
    $ ctest

Such a build requires the presence of [Google Test][7]. You can install this
yourself, or you can choose to instead rely on the [Conan][8] package manager
that the CI build infrastructure also uses. In that case, install Conan and do:

    $ conan install .. --build missing

in the build directory prior to running `cmake`. This will automatically
download and/or build Google Test.

For Windows, depending on the generator, you might also need to add switches
to select the architecture and build type, e.g.,

    $ conan install -s arch=x86_64 -s build_type=Debug ..

[7]: https://github.com/google/googletest
[8]: https://conan.io/

## Documentation

The documentation is still rather limited, and at the moment only available in
the sources (in the form of restructured text files in ``docs`` and Doxygen
comments in the header files). The intent is to automate the process of
building the documentation and have them available in more convenient formats
and in the usual locations.

## Building and Running the HelloWorld Example

We will show you how to build and run an example program that illustrates the
necessary steps to setup DCPS entities. The examples are built automatically
when you build the C++ language binding for Cyclone DDS, so you don't need to
follow these steps to be able to run the program, it is merely to illustrate
the process.

    $ mkdir helloworld
    $ cd helloworld
    $ cmake <install-location>/share/CycloneDDS-CXX/examples/helloworld
    $ cmake --build .

On one terminal start the application that will be responding to messages:

    $ ./ddscxxHelloWorldSubscriber

On another terminal, start the application that will be sending the messages:

    $ ./ddscxxHelloWorldPublisher

# Trademarks

 * "Eclipse Cyclone DDS" and "Cyclone DDS" are trademarks of the Eclipse Foundation.
 * "DDS" is a trademark of the Object Management Group, Inc.
 * "ROS" is a trademark of Open Source Robotics Foundation, Inc.

# License

This project contains 2 types of license: Apache2 and Eclipse Public License / Eclipse Distribution License
* The Apache2 license located in src/ddscxx/include/dds is for all files under src/ddscxx/include/dds except the details directories
* Eclipse Public License / Eclipse Distribution License is valid for all other files.
