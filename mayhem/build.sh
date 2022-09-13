#!/bin/bash -eu

#
# Copyright(c) 2006 to 2021 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#


# clone, build, and install cyclonedds first.
if [ -d cyclonedds ]; then rm -Rf cyclonedds; fi
git clone https://github.com/mayhemheroes/cyclonedds.git
cd cyclonedds
# remove oss-fuzz dir since we don't need it. Create blank CMakeLists.txt
rm -rf fuzz
mkdir fuzz
touch fuzz/CMakeLists.txt

mkdir build
cd build
cmake \
    -DBUILD_IDLC=ON \
    -DBUILD_TESTING=NO \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_EXAMPLES=ON \
    -DENABLE_SECURITY=ON \
    -DENABLE_SSL=NO \
    -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build .
cmake --build . --target install
cd ..

mkdir -p "$OUT/dds"
cp -vr ./build/bin/* "$OUT/dds/"

# move back up to cxx root and build bindings
cd ..

if [ -d build ]; then rm -Rf build; fi
mkdir build
cd build
cmake \
    -DBUILD_IDLLIB=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_PREFIX_PATH="/usr/local" \
    -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build .
cmake --build . --target install

cd ..
mkdir -p "$OUT/ddscxx"
cp -vr ./build/bin/* "$OUT/ddscxx/"
