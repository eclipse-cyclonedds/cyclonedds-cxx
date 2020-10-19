/***************************************************************************************/

  Copyright(c) 2006 to 2018 ADLINK Technology Limited and others

  This program and the accompanying materials are made available under the

  terms of the Eclipse Public License v. 2.0 which is available at

  http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License

  v. 1.0 which is available at

  http://www.eclipse.org/org/documents/edl-v10.php.

  SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

/***************************************************************************************/

# Description

This example mimics, in its most simplistic form, an Air Traffic Controller System , made of two subsystems:

A **Radar System** that, detects and publishes Flights in different region in Europe, within a dedicated Global data space, and an Air **Flight Visualiser system** that can build *different views* of that space, either Per country, Per Region or Per the Entire continent.



![](C:\Users\User\Documents\GitHub\cdds-cxx\src\examples\ATC\clip_image002.jpg)





# Design

The Radar system is modelled by the Publishing program ( **pubATC** ). This program

·     Get involved in the European Global Data Space DDS domain (domainId=0),

·     Creates 6 DDS partitions that corresponds to the 6 regions shown on the Map (see Fig below), then it

·     Publishes 5 Flights with different Call signs ( CIDs) as well as their respective positions .

The European Global Data Space is organized hierarchically following this pattern: /<Continent>/<Country>/<Region>.

The Air Flight Visualizer is modelled by the Subscribing program (**subATC**) . This program allows you to Subscribe to a specific region, for example, to the region /Europe/Sweden/Norrland or at the scale of the country ( /Europe/Italy/*) or the scale of the entire continent ( /*), Regular expressions can be used to specify the scopes .

The subATC program will create one subscriber to get all the data corresponding to the view you are building using the DDS concept of Partition.

The subATC program takes at most one argument. This argument specifies the absolute name of the region i.e “/<Continent>/<Country>/<Region>”

Both programs use the default DDS QoS but the PartitionQoS.

The example helps you understand the concept of partition in DDS and how it can be used to creates different views on the data or split the dds domain in smaller logical groups .

# Building the examples

cmake <CycloneDDS_Cpp_Dir>/examples/ATC

cmake --build .

# Running the examples

It is recommended to run one Publishing program ( pubATC) and several Subscribing programs in different terminals .

**Scenario1** : Subscribing to all the flights to have a global view on all the flights

\> ./pubATC

\>./subATC “*”

**Scenario2**  : Gross grain Subscription, subscribe to an entire country view

\> ./pubATC

\>./subATC /Europe/Netherlands/*

**Scenario3**  : Fine grain Subscription, subscribe to a give region

\> ./pubATC

\>./subATC /Europe/Netherlands/Zeeland

 

 