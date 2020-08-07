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
add_subdirectory(helloworld)

install(
  FILES helloworld/publisher.cpp
        helloworld/subscriber.cpp
        helloworld/HelloWorldData.idl
        helloworld/CMakeLists.txt
        helloworld/readme.rst
  DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/helloworld"
  COMPONENT dev)
