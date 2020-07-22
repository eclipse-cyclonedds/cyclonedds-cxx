# CXX API for CycloneDDS

This project provides a CXX API for [CycloneDDS](https://github.com/eclipse-cyclonedds/cyclonedds/)

[![Build Status](https://dev.azure.com/thijssassen/CXX-API/_apis/build/status/ThijsSassen.cdds-cxx?branchName=master)](https://dev.azure.com/thijssassen/CXX-API/_build/latest?definitionId=4&branchName=master)

## Requirements for the CXX API

In order to build the Idl compiler you need a Linux, Mac or Windows 10 machine with the following
installed on your host:

  * Git
  * [Conan](https://conan.io/), this is needed to install build dependencies
  * [CMake](https://cmake.org/download/), version 3.7 or later.  (Version 3.6 should work but you
    will have to edit the ``cmake_minimum_required`` version.)
  * [CycloneDDS](https://github.com/eclipse-cyclonedds/cyclonedds/)
  * [CXX Idl compiler](https://github.com/ADLINK-IST/idlpp-cxx/)


## Building

Building the CXX API, requires only a few simple steps. There are some small differences
between Linux and macOS on the one hand, and Windows on the other. For Linux or macOS:

    $ git clone https://github.com/ThijsSassen/cdds-cxx.git
    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_PREFIX_PATH="<idlpp-cxx install path>/lib/cmake/Idlpp-cxx;<CycloneDDS install path>/lib/cmake/CycloneDDS" <cmake-config_options> ..
    $ cmake --build .

and for Windows:

    $ git clone https://github.com/ThijsSassen/cdds-cxx.git
    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_PREFIX_PATH="<idlpp-cxx install path>\lib\cmake\Idlpp-cxx;<CycloneDDS install path>\lib\cmake\CycloneDDS" -G "<generator-name>" <cmake-config_options> ..
    $ cmake --build .

where you replace ``<generator-name>`` by one of the ways
CMake [generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) offer for
generating build files.  For example, "Visual Studio 15 2017 Win64" would target a 64-bit build
using Visual Studio 2017.

The ``<cmake-config_options>`` can be ignored or replaced. A few of the most common options are:
-DCMAKE_INSTALL_PREFIX=``<install-location>``
-DCMAKE_BUILD_TYPE=Debug
-DBUILD_TESTING=ON, to enable testing
-DUSE_DOCS=1, to generate documentation

    $ cmake -DCMAKE_BUILD_TYPE=Debug ..


## Packaging

If you want to package the product, the config step and build step are slightly different compared
to how it is normally build.

The -DCMAKE_INSTALL_PREFIX=``<install-location>`` option should be added to the configuration,
where the ``<install-location>`` is replaced by the directory under which you would like to
install the Idl compiler.

During the build step, you have to specify that you want to build the install target as well.


This would make the build look like

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_PREFIX_PATH="<idlpp-cxx install path>/lib/cmake/Idlpp-cxx;<CycloneDDS install path>/lib/cmake/CycloneDDS" -DCMAKE_INSTALL_PREFIX=<install-location>  ..
    $ cmake --build . --target install

Don't forget the generator when building on Windows.

After the build, required files are copied to:

  * ``<install-location>/lib``
  * ``<install-location>/share``

The ``<install-location>`` directory is will be used to create the package(s).

    $ cpack

Depending on the target, you now have packages.

## Documentation

Documentation for the CXX API can be found [here](https://atolab.github.io/cdds-docs/api/cxx/index.html)

## License

This project contains 2 types of license: Apache2 and Eclipse Public License / Eclipse Distribution License
* The Apache2 license located in src/ddscxx/include/dds is for all files under src/ddscxx/include/dds except the details directories
* Eclipse Public License / Eclipse Distribution License is valid for all other files.
