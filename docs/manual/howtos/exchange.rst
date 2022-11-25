..
   Copyright(c) 2022 ZettaScale Technology and others

   This program and the accompanying materials are made available under the
   terms of the Eclipse Public License v. 2.0 which is available at
   http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
   v. 1.0 which is available at
   http://www.eclipse.org/org/documents/edl-v10.php.

   SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

Exchanging Data
===============

This guide describes some basic concepts behind DDS data exchange.

DomainParticipant
-----------------

A Domain is a specific subsection of the DDS shared-dataspace and identified by its domain ID, which is a 32-bit unsigned integer.
Data exchanges are limited to the domain they are made on, for example, data exchanged on domain 456 is not visible on domain 789.
To exchange data you must create a DomainParticipant, which is an entrypoint for the program on the shared dataspace's domain.

You can either specify the default domain ID:

.. code:: C++

	dds::domain::DomainParticipant participant(domain::default_id());

or, to have more control over the process, select your own ID:

.. code:: C++

	dds::domain::DomainParticipant participant(123456);

The main point is that you must have the same ID on both the reading side and the writing side, otherwise, they can not see each other.
???Explain more about setting QoSes???

Topic
-----

A Topic is a subsection of a DDS Domain that enables exchange of data of a specific type, and that complies with certain restrictions on the exchange before exchange can occur. 
A Topic is identifiable by:

- Name
	- identifies the topic on the Domain
	- must be unique on the Domain
- Type
	- the type of data being exchanged
	- is the template parameter of the dds::topic::Topic class
- Quality of Service
	- determines the restrictions on the exchange
	- this is an optional parameter, and can be derived from fallbacks to the participant or defaults

A Topic is for exchanging data of the Type Data_Type. The following shows how a topic is created on the DomainParticipant participant:

.. code:: C++

	dds::topic::Topic<Data_Type> topic(participant, "DataType Topic");

To generate the data type of the topic from the user's IDL files, use CycloneDDS's idlc generator (with the idlcxx library).
Using types other than those generated from idlc+idlcxx in the template does not have the prerequisite traits, and therefore does not result in working code.

Publishers
----------

A Publisher is a producer of data on a Domain. It uses the DomainParticipant to gain access to the Domain and is created using it. That is, the Publisher passes down the Domain from its parent class DomainParticipant. A Publisher allows the DataWriters associated with it to share the same behaviour, for example:

- liveliness notifications
- QoS policies
- listener callbacks
- etc.

You can either use the default settings:

.. code:: C++

	dds::pub::Publisher pub(participant);

Or supply your own:

.. code:: C++

	dds::pub::NoOpPublisherListener listener; /*you need to create your own class that derives from this listener, and implement your own callbacks*/
	/*the listener implementation should implement the on_publication_matched virtual function as we will rely on it later*/
	dds::pub::qos::PublisherQos pubqos; /*add custom QoS policies that you want for this publisher*/
	dds::pub::Publisher pub(participant, pubqos, &listener, dds::core::status::StatusMask::publication_matched()); /*in this case, the only status we are interested in is publication_matched*/

Any DataWriters created using pub inherit the qos and listener functionality as set through it.

Subscribers
-----------

A Subscriber is a consumer of data on a Domain. It uses the DomainParticipant to gain access to the Domain and is created using it.
A Subscriber allows the DataReaders associated with it to share the same behaviour, such as:

- liveliness notifications
- QoS policies
- listener callbacks
- etc.

You can either use the default settings:

.. code:: C++

	dds::sub::Subscriber sub(participant);

Or supply your own:

.. code:: C++

	dds::sub::NoOpSubscriberListener listener; /*you need to create your own class that derives from this listener, and implement your own callbacks*/
	/*the listener implementation should implement the on_subscription_matched virtual function as we will rely on it later*/
	dds::sub::qos::SubscriberQos subqos; /*add custom QoS policies that you want for this subscriber*/
	dds::sub::Subscriber sub(participant, subqos, &listener, dds::core::status::StatusMask::subscription_matched());

Any DataReaders created using sub inherit the qos and listener functionality as set through it.

DataReaders
-----------

DataReaders enable the user access to the data received by a Subscriber on a Topic, and takes as a template parameter the data type being exchanged. The settings for the reader are:

Either, inheriting from the subscriber:

.. code:: C++

	dds::sub::DataReader<DataType> reader(sub, topic);

Or, explicitly setting its own QoS policies and listener:

.. code:: C++

	dds::sub::NoOpAnyDataReaderListener listener; /*you need to create your own class that derives from this listener, and implement your own callback functions*/
	/*the listener implementation should implement the on_data_available virtual function as we will rely on it later*/
	dds::sub::qos::DataReaderQos rqos;
	dds::sub::DataReader<DataType> reader(sub, topic, rqos, &listener, dds::core::status::StatusMask::data_available());

The data is accessed by either `reading` or `taking` the samples from the reader.
Both return a container of `dds::sub::Sample`s which have the received sample of the exchanged datatype accessed through `data()` and the metadata for the received sample accessed through `info()`.
The metadata contains such information as:

- sample timestamp (time of writing)
- data validity (whether the call to `data()` will return anything that should be processed)
- sample state (READ/NOT_READ/...)
- ...

The difference between these two different access methods is the state of the reader after the access is finished.
The `take` operation only returns samples which have not yet been returned in a `take` operation, whereas the `read` operation returns all samples currently stored by the reader.

.. code:: C++

	auto samples = reader.take();
	for (const auto & sample:samples) {
		if (!sample.valid())
			continue;
		const auto &data = sample.data();
		/*print the data?*/
	}

.. code:: C++

	auto samples = reader.read();
	for (const auto & sample:samples) {
		if (!sample.valid() ||
			sample.state() != dds::sub::status::SampleState::not_read())
			continue;
		const auto &data = sample.data();
		/*print the data?*/
	}

DataWriters
-----------

DataWriters allow the user to write data to a Topic using a Publisher, and take as a template parameter the data type being exchanged. The settings for the writer are either inheriting from the publisher:

.. code:: C++

	dds::pub::DataWriter<DataType> writer(pub, topic);

, or explicitly setting its own QoS policies and listener:

.. code:: C++

	dds::pub::NoOpAnyDataWriterListener listener; /*you need to create your own class that derives from this listener, and implement your own callback functions*/
	/*the listener implementation should implement the on_publication_matched virtual function as we will rely on it later*/
	dds::pub::qos::DataWriterQos wqos;
	dds::pub::DataWriter<DataType> writer(pub, topic, wqos, &listener, dds::core::status::StatusMask::publication_matched());

A writer can simply write a sample:

.. code:: C++

	DataType sample;
	writer.write(sample);

A sample with a specific timestamp:

.. code:: C++

	DataType sample;
	dds::core::Time timestamp(123 /*seconds*/, 456 /*nanoseconds*/);
	writer.write(sample, timestamp);

Or a range of samples:

.. code:: C++

	std::vector<DataType> samples;
	writer.write(samples.begin(), samples.end());

Or update existing instances through handles, which we will not go into here.

Small Example
-------------

Putting it all together, we can create the following code for writing data of the type DataType:

.. code:: C++

	/* for std::this_thread */
	#include <thread>

	/* include C++ DDS API. */
	#include "dds/dds.hpp"

	/* include the c++ data type, generated from idlcxx */
	#include "DataType.hpp"

	using namespace org::eclipse::cyclonedds;

	int main() {
		/*errors in construction/etc are indicated by exceptions*/
		try {
			dds::domain::DomainParticipant participant(domain::default_id());

			dds::topic::Topic<DataType> topic(participant, "DataType Topic");

			dds::pub::Publisher publisher(participant);

			dds::pub::DataWriter<DataType> writer(publisher, topic);

			/*we wait for a reader to appear*/
			while (writer.publication_matched_status().current_count() == 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(20));

			DataType msg;

			/*modify msg*/

			writer.write(msg);

            /*we wait for the reader to disappear*/
			while (writer.publication_matched_status().current_count() > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
		} catch (const dds::core::Exception& e) {
			std::cerr << "An exception occurred: " << e.what() << std::endl;
			exit(1);
		}
		return 0;
	}

This writer waits for a reader to appear and then writes a single sample to the DDS service, after that, it waits for the reader to disappear and then exits.
And for reading data:

.. code:: C++

	/* for std::this_thread */
	#include <thread>

	/* include C++ DDS API. */
	#include "dds/dds.hpp"

	/* include the c++ data type, generated from idlcxx */
	#include "DataType.hpp"

	using namespace org::eclipse::cyclonedds;

	int main() {

		/*errors in construction/etc are indicated by exceptions*/
		try {
			dds::domain::DomainParticipant participant(domain::default_id());

			dds::topic::Topic<DataType> topic(participant, "DataType Topic");

			dds::sub::Subscriber subscriber(participant);

			dds::sub::DataReader<DataType> reader(subscriber, topic);

			/*we periodically check the reader for new samples*/
			bool reading = true;
			while (reading) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				auto samples = reader.take();
				for (const auto & p:samples) {
					const auto& info = p.info(); /*metadata*/
					if (info.valid()) {
						/*this sample contains valid data*/
						const auto& msg = p.data(); /* the actual data */
						std::cout << "Message received." << std::endl;
						reading = false; /*we are done reading*/
					}
				}
			}
		} catch (const dds::core::Exception& e) {
			std::cerr << "An exception occurred: " << e.what() << std::endl;
			exit(1);
		}
		return 0;
	}

The reader periodically (every 20ms) checks for received data, and stops when it receives some.
