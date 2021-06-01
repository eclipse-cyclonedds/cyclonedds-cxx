..
   Copyright(c) 2006 to 2018 ADLINK Technology Limited and others

   This program and the accompanying materials are made available under the
   terms of the Eclipse Public License v. 2.0 which is available at
   http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
   v. 1.0 which is available at
   http://www.eclipse.org/org/documents/edl-v10.php.

   SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

HelloWorld SHM
==========

Description
***********

The basic ddscxx HelloWorldShm example is used to illustrate the necessary steps to setup DCPS entities for using shared memory

Design
******

It consists of 2 units:

- ddscxxHelloworldPublisherShm: implements the publisher's main
- ddscxxHelloworldSubscriberShm: implements the subscriber's main

Scenario
********

The publisher sends a single HelloWorld sample. The sample contains the following field:

- a userID field (long type)

When it receives the sample, the subscriber displays the userID field.

Running the example
*******************

Configuration
--------------

Cyclone DDS  needs to be configured to use the shared memory exchange

Below is an example of Cyclone DDS configuration file to enable shared memory exchange:

.. code-block:: xml

  <?xml version="1.0" encoding="UTF-8" ?>
  <CycloneDDS xmlns="https://cdds.io/config"
              xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
              xsi:schemaLocation="https://cdds.io/config https://raw.githubusercontent.com/eclipse-cyclonedds/cyclonedds/iceoryx/etc/cyclonedds.xsd">
      <Domain id="any">
          <SharedMemory>
              <Enable>true</Enable>
              <SubQueueCapacity>256</SubQueueCapacity>
              <SubHistoryRequest>16</SubHistoryRequest>
              <PubHistoryCapacity>16</PubHistoryCapacity>
              <LogLevel>info</LogLevel>
          </SharedMemory>
      </Domain>
  </CycloneDDS>

The above example configuration can be saved as *cyclonedds.xml* and can be passed to Cyclone DDS through the environment variable CYCLONEDDS_URI as below

.. code-block:: bash

  export CYCLONEDDS_URI=file://cyclonedds.xml

Run
--------------

It is recommended that you run the executables in separate terminals to avoid mixing the output.

- In the first terminal start the RouDi by running iox-roudi

.. code-block:: bash

  ~/iceoryx/build/iox-roudi

- In the second terminal start the subscriber by running ddscxxHelloWorldSubscriberShm

.. code-block:: bash

  export CYCLONEDDS_URI=file://cyclonedds.xml
  ~/cyclonedds-cxx/build/bin/ddscxxHelloWorldSubscriberShm

- In the third terminal start the publisher by running ddscxxHelloWorldPublisherShm

.. code-block:: bash

  export CYCLONEDDS_URI=file://cyclonedds.xml
  ~/cyclonedds-cxx/build/bin/ddscxxHelloWorldPublisherShm

After establishing a successful communication, the output looks something like below:

.. code-block:: bash

  === [Subscriber] Wait for message.
  === [Subscriber] Message received:
      data  : 1234
  === [Subscriber] Done.


  
