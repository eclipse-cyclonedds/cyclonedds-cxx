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

#include <iostream>
#include "dds/dds.hpp"
#include "dds/core/ddscore.hpp"
#include "ATCDataModel_DCPS.hpp"

int main(int argc, char *argv[]) {

	std::cout << "Cyclone- ATC Pub" << std::endl;

	/** A dds::domain::DomainParticipant is created for the default domain. */
	dds::domain::DomainParticipant dp(0);
	dds::topic::qos::TopicQos topicQos= dp.default_topic_qos();

	/** A dds::topic::Topic is created for our sample type on the domain participant. */
	dds::topic::Topic<ATCDataModule::Flight> topic(dp, "FlightData", topicQos);

	/** 5 dds Publishers are associated to 5 different partitions. */
	dds::pub::qos::PublisherQos PubGotalandQoS = dp.default_publisher_qos();
	dds::pub::qos::PublisherQos PubSardegnaQoS = dp.default_publisher_qos();
	dds::pub::qos::PublisherQos PubNorrlandQoS = dp.default_publisher_qos();
	dds::pub::qos::PublisherQos PubLombardiaQoS = dp.default_publisher_qos();
	dds::pub::qos::PublisherQos pubSiciliaQoS = dp.default_publisher_qos();

	dds::pub::qos::PublisherQos pubZeelandQoS = dp.default_publisher_qos();
	dds::pub::qos::PublisherQos pubDrentheQoS = dp.default_publisher_qos();

	PubGotalandQoS  <<  dds::core::policy::Partition("/Europe/Sweden/Gotaland");
	PubNorrlandQoS  <<  dds::core::policy::Partition("/Europe/Sweden/Norrland");
	PubSardegnaQoS  <<  dds::core::policy::Partition("/Europe/Italy/Sardegna");
	PubLombardiaQoS <<  dds::core::policy::Partition("/Europe/Italy/Lombardia");
	pubSiciliaQoS   <<  dds::core::policy::Partition("/Europe/Italy/Sicilia");
	pubZeelandQoS   << dds::core::policy::Partition("/Europe/Netherlands/Zeeland");
	pubDrentheQoS   << dds::core::policy::Partition("/Europe/Netherlands/Drenthe");

	dds::pub::Publisher pubGotaland(dp, PubGotalandQoS);
	dds::pub::Publisher pubNorrland(dp, PubNorrlandQoS);
	dds::pub::Publisher pubSardegna(dp, PubSardegnaQoS);
	dds::pub::Publisher pubLombardia(dp, PubLombardiaQoS);
	dds::pub::Publisher pubSicilia(dp, pubSiciliaQoS);
	dds::pub::Publisher pubZeeland(dp, pubZeelandQoS);
	dds::pub::Publisher pubDrenthe(dp, pubDrentheQoS);

	/** A dds::pub::DataWriter is created on the Publisher & Topic with the associated Partition Qos. */
	dds::pub::qos::DataWriterQos dwqos = topic.qos();

	dds::pub::DataWriter<ATCDataModule::Flight> dwGotaland(pubGotaland, topic, dwqos);
	dds::pub::DataWriter<ATCDataModule::Flight> dwNorrland(pubNorrland, topic, dwqos);
	dds::pub::DataWriter<ATCDataModule::Flight> dwSardegna(pubSardegna, topic, dwqos);
	dds::pub::DataWriter<ATCDataModule::Flight> dwLombardia(pubLombardia, topic, dwqos);
	dds::pub::DataWriter<ATCDataModule::Flight> dwsicilia(pubSicilia, topic, dwqos);
	dds::pub::DataWriter<ATCDataModule::Flight> dwzeeland(pubZeeland, topic, dwqos); 
	dds::pub::DataWriter<ATCDataModule::Flight> dwdrenthe(pubDrenthe, topic, dwqos); 

 	// std::cout << "@ With ddsi, Let the Apps discover each other a bit ZZZzzz:" << std::endl;
	//dds_sleepfor (DDS_MSECS (5000));

	std::cout << "===> Waiting for a subscriber. " << std::endl;
    while (dwdrenthe.publication_matched_status().current_count() == 0 && 
					dwzeeland.publication_matched_status().current_count() == 0 && 
					dwsicilia.publication_matched_status().current_count() == 0 && 
					dwLombardia.publication_matched_status().current_count() == 0 && 
					dwSardegna.publication_matched_status().current_count() == 0 && 
					dwNorrland.publication_matched_status().current_count() == 0 && 
					dwGotaland.publication_matched_status().current_count() == 0
					) 
			{
						dds_sleepfor (DDS_MSECS (20));
			}
	std::cout << "===> Found a matching one ... " << std::endl;

	for (int i = 0; i < 5; i++) {
		ATCDataModule::Position p1(i+800, i+20, i+93 );
		ATCDataModule::Flight FlightInstance(i, p1,"** /Europe/Sweden/Gotaland partition ** ");
		dwGotaland << FlightInstance;
		std::cout << "=> [Radar Simulator] detecting FlightID: "<< FlightInstance.ID() << " in  the /Europe/Sweden/Gotaland  Region  " << std::endl;
		
		dds_sleepfor (DDS_MSECS (500));
	}
          
	for (int i = 5; i < 10; i++) {
		ATCDataModule::Position p2(i+7900, i+20, i+93 );
		ATCDataModule::Flight FlightInstance(i, p2, "** /Europe/Sweden/Norrland partition ++");
		dwNorrland << FlightInstance;
		std::cout << "=> [Radar Simulator] detecting FlightID: "<< FlightInstance.ID() << " in  the /Europe/Sweden/Norrland  Region  " << std::endl;
		
		dds_sleepfor (DDS_MSECS (500));
	}
	
	for (int i = 10; i < 15; i++) {
 		ATCDataModule::Position p3(i+800, i+20, i+93 );
		ATCDataModule::Flight FlightInstance(i, p3, "** /Europe/Italy/Sardegna partition &&");
		dwSardegna << FlightInstance;
		std::cout << "=> [Radar Simulator] detecting FlightID: "<< FlightInstance.ID() << " in the /Europe/Italy/Sardegna   Region  " << std::endl;

		dds_sleepfor (DDS_MSECS (500));	
}

	for (int i = 15; i < 20; i++) {
 		ATCDataModule::Position p4(i+99, i+97, i+93 );
		ATCDataModule::Flight FlightInstance(i, p4, "** /Europe/Italy/Lombardia partition %%");
		dwLombardia << FlightInstance;
		std::cout << "=> [Radar Simulator] detecting FlightID: "<< FlightInstance.ID() << " in the /Europe/Italy/Lombardia  Region  " << std::endl;
	
		dds_sleepfor (DDS_MSECS (500));
	}

	for (int i = 20; i < 25; i++) {
 		ATCDataModule::Position p5(i+242, i+240, i+930 );
		ATCDataModule::Flight FlightInstance(i,p5, "** /Europe/Italy/Sicilia partition $$");
		dwsicilia << FlightInstance;
		std::cout << "=> [Radar Simulator] detecting FlightID: "<< FlightInstance.ID() << " in the /Europe/Italy/Sicilia    Region  " << std::endl;

		dds_sleepfor (DDS_MSECS (500));
	}

	for (int i = 25; i < 30; i++) {
 		ATCDataModule::Position p6(i+22, i+20, i+930 );
		ATCDataModule::Flight FlightInstance(i,p6, "** /Europe/Netherlands/Zeeland partition &&");
		dwzeeland << FlightInstance;
		std::cout << "=> [Radar Simulator] detecting FlightID: "<< FlightInstance.ID() << " in the /Europe/Netherlands/Zeeland    Region  " << std::endl;
		
		dds_sleepfor (DDS_MSECS (500));
	}
 
	for (int i = 30; i < 35; i++) {
 		ATCDataModule::Position p7(i+22, i+20, i+930 );
		ATCDataModule::Flight FlightInstance(i,p7, "** /Europe/Netherlands/Drenthe partition &&");
		dwdrenthe << FlightInstance;
		std::cout << "=> [Radar Simulator] detecting FlightID: "<< FlightInstance.ID() << " in the /Europe/Netherlands/Drenthe    Region  " << std::endl;

		dds_sleepfor (DDS_MSECS (500));
	}

	
	dds_sleepfor (DDS_MSECS (5000));
	return 0;
}

