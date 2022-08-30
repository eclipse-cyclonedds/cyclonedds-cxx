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

(
#build and install cyclonedds first.
if [ -d cyclonedds ]; then rm -Rf cyclonedds; fi
git clone https://github.com/mayhemheroes/cyclonedds.git
cd cyclonedds

if [ -d build ]; then rm -Rf build; fi
./mayhem/build.sh

#back up to cxx root.
cd ..
rm -rf cyclonedds

if [ -d build ]; then rm -Rf build; fi
mkdir build
cd build
cmake \
    -DBUILD_IDLLIB=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_EXAMPLES=NO \
    -DCMAKE_PREFIX_PATH="/usr/local" \
    -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build .
cmake --build . --target install
cd ..
)
