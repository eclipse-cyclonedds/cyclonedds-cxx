/*                CycloneDDS isoC++ example 
 *
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 *
 */




#include<iostream>
//#include "common/example_utilities.h"
#include "ATCDataModel_DCPS.hpp"

// ********************* Subscriber program : Pooling *********************
//                       Partition QoSs is used 
int main(int argc, char* argv[])
{
	std::cout << " Cyclone- ATC Subscriber, Flight Visualiser per Country or Region *** " << std::endl;
	

	/** A domain participant and topic are created identically as in the ::publisher */
	//dds::domain::DomainParticipant dp(org::opensplice::domain::default_id());
	dds::domain::DomainParticipant dp(0);
	dds::topic::qos::TopicQos topicQos = dp.default_topic_qos();
	dds::topic::Topic<ATCDataModule::Flight> topic(dp, "FlightData", topicQos);
	
	/** A dds::pub::Sub is created on the domain participant. */
	std::string PartitionName;
	

	if(argc<2) 
	{

	std::cout << "=< Specify the Region Name (Partition) , a regular expression with * or ? " << std::endl;
	std::cout << "=< 	for examples :*;/* ; /E*; /Europe/*; or ..." << std::endl;
	std::cout << "=<                   :  /Europe/Italy/*; /Europe/Italy/Sardegna; /Europe/Italy/Sicilia; /Europe/Netherlands/*, ..." << std::endl;
	std::cout << "=<  Enter the Region Name or Expression:" << std::endl;

	std::cin >> PartitionName;
	std::cout << " Thanks !" << std::endl;

	}
	else{
           
	PartitionName=argv[1];

	}
           //std::cout << " The partition is..:" << PartitionName <<std::endl;
	dds::sub::qos::SubscriberQos  subQos = dp.default_subscriber_qos() << dds::core::policy::Partition(PartitionName);;
	dds::sub::Subscriber sub(dp, subQos);
	
	/** A dds::sub::DataReader is created on the Subscriber & Topic with the DataReaderQos. */
	dds::sub::qos::DataReaderQos drqos = topic.qos();

	dds::sub::DataReader<ATCDataModule::Flight> dr(sub, topic, drqos);
	bool sampleReceived = false;
	// The Subscriber is going to wait during 120 seconds then leaves , to allow receiving all the instances and their samples
	int count = 0;
	do
	{
		dds::sub::LoanedSamples<ATCDataModule::Flight> samples = dr.take();
		for (dds::sub::LoanedSamples<ATCDataModule::Flight>::const_iterator sample = samples.begin(); sample < samples.end(); ++sample)
		{
			if (sample->info().valid())
			{
				std::cout << "=== [Flight Visualiser] Flight with ID :" << sample->data().ID() <<" Visualized"<<std::endl;
				std::cout << "                        In the Region of: \"" << sample->data().CurrentRegion() << "\"" << std::endl;
				sampleReceived = true;
			}
			else {
				// std::cout << "data disposed, explicitely or the DW lost his liveliness" << std::endl;
			}
		}
		//exampleSleepMilliseconds(1);
		dds_sleepfor (DDS_MSECS (1));
		++count;
	} while (count < 120000 ) ;


	if (!sampleReceived)
	{
		std::cerr << "Warning: Waited too long (120 seconds) but no sample received" << std::endl;
	}
	return 0;
}
