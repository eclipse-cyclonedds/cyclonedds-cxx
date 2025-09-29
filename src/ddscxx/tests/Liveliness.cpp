// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "Util.hpp"
#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "HelloWorldData.hpp"
#include "Space.hpp"


/**
 * Fixture for the Liveliness tests
 */
class Liveliness : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::pub::Publisher publisher;
    dds::sub::Subscriber subscriber;

    Liveliness() :
        participant(dds::core::null),
        publisher(dds::core::null),
        subscriber(dds::core::null)
    {
    }

    void SetUp()
    {
        this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(this->participant, dds::core::null);
        this->publisher = dds::pub::Publisher(this->participant);
        ASSERT_NE(this->publisher, dds::core::null);
        this->subscriber = dds::sub::Subscriber(this->participant);
        ASSERT_NE(this->subscriber, dds::core::null);
    }

    void TearDown()
    {
        this->publisher = dds::core::null;
        this->participant = dds::core::null;
    }

};



/**
 * Tests
 */

TEST_F(Liveliness, manual_liveliness)
{
    Space::Type1 testData1(1,1,1);
    dds::core::Duration ldur(0, DDS_MSECS(500));
    dds::core::status::LivelinessChangedStatus lstatus;

    dds::topic::Topic<Space::Type1> topic = dds::topic::Topic<Space::Type1>(this->participant, "liveliness_test_topic");
    ASSERT_NE(topic, dds::core::null);

    for (uint32_t n = 0; n <= 1; n++)
    {
        // Create writer with liveliness set to manual-by-topic / manual-by-participant, duration ldur
        dds::pub::qos::DataWriterQos qos = this->publisher.default_datawriter_qos();
        if (n == 0)
            qos << dds::core::policy::Liveliness::ManualByTopic(ldur);
        else
            qos << dds::core::policy::Liveliness::ManualByParticipant(ldur);
        dds::pub::DataWriter<Space::Type1> writer = dds::pub::DataWriter<Space::Type1>(this->publisher, topic, qos);
        ASSERT_NE(writer, dds::core::null);

        // Create reader
        dds::sub::DataReader<Space::Type1> reader = dds::sub::DataReader<Space::Type1>(this->subscriber, topic);
        ASSERT_NE(reader, dds::core::null);

        // Wait for the writer to become alive
        do {
            lstatus = reader.liveliness_changed_status();
            dds_sleepfor(DDS_MSECS(10));
        }
        while (lstatus.alive_count() != 1);

        // Write a sample, wait for 1.5 * ldur and expect the writer to be not-alive
        writer.write(testData1);
        dds_sleepfor(3 * DDS_MSECS(ldur.to_millisecs()) / 2);
        lstatus = reader.liveliness_changed_status();
        ASSERT_EQ(lstatus.alive_count(), 0);

        // Write a sample, sleep for 0.6 * ldur and expect the writer to be alive
        writer.write(testData1);
        dds_sleepfor(6 * DDS_MSECS(ldur.to_millisecs()) / 10);
        lstatus = reader.liveliness_changed_status();
        ASSERT_EQ(lstatus.alive_count(), 1);

        // Assert liveliness
        if (n == 0)
            writer.assert_liveliness();
        else
            this->participant->assert_liveliness();

        // Sleep another 0.6 * ldur and expect the writer to be alive
        dds_sleepfor(6 * DDS_MSECS(ldur.to_millisecs()) / 10);
        lstatus = reader.liveliness_changed_status();
        ASSERT_EQ(lstatus.alive_count(), 1);
    }
}


