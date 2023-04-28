// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <gtest/gtest.h>

#include "dds/dds.hpp"
#include "Space.hpp"

/* Class to test implicit conversion from any Condition to Condition and vice versa. */
class ConditionStore
{
public:
    ConditionStore(const dds::core::cond::Condition& c)
        : cond(c)
    {
    }
    const dds::core::cond::Condition& get() {
        return cond;
    }
private:
    const dds::core::cond::Condition cond;
};

class FunctorCondition {
public:
    FunctorCondition(bool& handlerExecuted)
        : handlerExecuted(handlerExecuted) { }

    void operator() () {
        handlerExecuted = true;
    }

    void operator ()(dds::core::cond::Condition&) {
        handlerExecuted = true;
    }
private:
    bool& handlerExecuted;
};

class FunctorStatusCondition {
public:
    void operator ()(dds::core::cond::StatusCondition) {
        /* Empty. */
    }
};

class FunctorGuardCondition
{
public:
    void operator() (dds::core::cond::Condition&)
    {
        /* Empty. */
    }
};

class FunctorReadCondition {
public:
    void operator ()(dds::sub::cond::ReadCondition) {
        /* Empty. */
    }
};

class FunctorReadConditionSample {
public:
    FunctorReadConditionSample(const Space::Type1 &sample) : expectedSample(sample) 
    { }

    void operator ()(const dds::sub::cond::ReadCondition &cond) {
        dds::sub::DataReader<Space::Type1> reader = cond.data_reader();
        dds::sub::LoanedSamples<Space::Type1> samples = reader.take();
        ASSERT_GT(samples.length(), 0u) << "No samples returned";
        ASSERT_EQ((*samples.begin()).data(), expectedSample) << "The returned sample is incorrect";
    }

private:
    const Space::Type1 &expectedSample;
};

// Disabled because QueryCondition currently not supported
// class FunctorQueryCondition {
// public:
//     void operator ()(dds::sub::cond::QueryCondition) {
//         /* Empty. */
//     }
// };

template <typename T>
void wait_for_data(dds::sub::DataReader<T> dr)
{
    // Get current reader status
    dds::core::cond::StatusCondition readerStatus(dr);
    dds::core::status::StatusMask status = readerStatus.enabled_statuses();

    // Set enabled mask to data_available
    dds::core::status::StatusMask statusMask;
    statusMask << dds::core::status::StatusMask::data_available();
    readerStatus.enabled_statuses(statusMask);

    // Wait for data
    dds::core::cond::WaitSet w;
    w += readerStatus;
    w.wait(dds::core::Duration::from_millisecs(1500));

    // Reset enabled statuses
    readerStatus.enabled_statuses(status);
}


/**
 * Fixture for the tests
 */
class Condition : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::pub::Publisher publisher;
    dds::sub::Subscriber subscriber;
    dds::topic::Topic<Space::Type1> topic;
    dds::pub::DataWriter<Space::Type1> writer;
    dds::sub::DataReader<Space::Type1> reader;
    // dds::sub::Query query;
    std::string partition;
    FunctorStatusCondition functor_status_cond;
    FunctorGuardCondition functor_guard_cond;
    FunctorReadCondition functor_read_cond;
    // FunctorQueryCondition functor_query_cond;
    bool handlerExecuted;
    FunctorCondition functor_cond;

    Condition() :
        participant(dds::core::null),
        publisher(dds::core::null),
        subscriber(dds::core::null),
        topic(dds::core::null),
        writer(dds::core::null),
        reader(dds::core::null),
        // query(dds::core::null),
        partition("Condition_test"),
        handlerExecuted(false),
        functor_cond(handlerExecuted)
    {
    }

    void SetUp()
    {
        this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(this->participant, dds::core::null);

        dds::pub::qos::PublisherQos pubQos =
                this->participant.default_publisher_qos() <<
                dds::core::policy::Partition(this->partition);
        this->publisher = dds::pub::Publisher(this->participant, pubQos);
        ASSERT_NE(this->publisher, dds::core::null);

        dds::sub::qos::SubscriberQos subQos =
                this->participant.default_subscriber_qos() <<
                dds::core::policy::Partition(this->partition);
        this->subscriber = dds::sub::Subscriber(this->participant, subQos);
        ASSERT_NE(this->subscriber, dds::core::null);

        this->topic = dds::topic::Topic<Space::Type1>(this->participant, "condition_test_topic");
        ASSERT_NE(this->topic, dds::core::null);

        dds::pub::qos::DataWriterQos writerQos =
                this->publisher.default_datawriter_qos() <<
                dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
        this->writer = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic, writerQos);
        ASSERT_NE(this->writer, dds::core::null);

        this->reader = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic);
        ASSERT_NE(this->reader, dds::core::null);
    }

    void TearDown()
    {
        this->reader = dds::core::null;
        this->writer = dds::core::null;
        this->topic = dds::core::null;
        this->publisher = dds::core::null;
        this->subscriber = dds::core::null;
        this->participant = dds::core::null;
        // this->query = dds::core::null;
    }
};

/**
 * Test creating conditions (status, guard and read)
 */
TEST_F(Condition, create)
{
    dds::core::cond::StatusCondition status_cond = dds::core::null;
    dds::core::cond::GuardCondition guard_cond = dds::core::null;
    // dds::sub::cond::QueryCondition query_cond = dds::core::null;
    dds::sub::cond::ReadCondition read_cond = dds::core::null;

    status_cond = dds::core::cond::StatusCondition(participant);
    guard_cond = dds::core::cond::GuardCondition();
    // QueryCondition is currently not suppored
    // query_cond = dds::sub::cond::QueryCondition(query, dds::sub::status::DataState::any());
    read_cond = dds::sub::cond::ReadCondition(reader, dds::sub::status::DataState::any());

    ASSERT_FALSE(status_cond == dds::core::null) << "StatusCondition object null after creation";
    ASSERT_FALSE(guard_cond == dds::core::null) << "GuardCondition object null after creation";
    // ASSERT_FALSE(query_cond == dds::core::null);
    ASSERT_FALSE(read_cond == dds::core::null) << "ReadCondition object null after creation";
}

/**
 * Test handlers for conditions (tested on GuardCondition)
 */
TEST_F(Condition, handler)
{
    dds::core::cond::GuardCondition guard_cond = dds::core::null;
    dds::core::cond::GuardCondition guard_cond2 = dds::core::null;
    dds::core::cond::WaitSet waitSet = dds::core::null;

    // Create a waitset and add the guard condition
    waitSet = dds::core::cond::WaitSet();
    guard_cond = dds::core::cond::GuardCondition();
    waitSet += guard_cond;

    // Trigger guard, dispatch
    handlerExecuted = false;
    guard_cond.trigger_value(true);
    waitSet.dispatch();
    ASSERT_FALSE(handlerExecuted) << "GuardCondition functor should not be executed when not attached";
    guard_cond.trigger_value(false);

    // Add handler
    handlerExecuted = false;
    guard_cond.handler(functor_cond);
    guard_cond.trigger_value(true);
    waitSet.dispatch();
    ASSERT_TRUE(handlerExecuted) << "Attached GuardCondition functor was not triggered";
    guard_cond.trigger_value(false);

    // Reset handler 
    handlerExecuted = false;
    guard_cond.reset_handler();
    guard_cond.trigger_value(true);
    waitSet.dispatch();
    ASSERT_FALSE(handlerExecuted) << "GuardCondition functor should not be executed when reset";
    guard_cond.trigger_value(false);

    // Add functor in constructor
    guard_cond2 = dds::core::cond::GuardCondition(functor_cond);
    waitSet -= guard_cond;
    waitSet += guard_cond2;

    handlerExecuted = false;
    guard_cond2.trigger_value(true);
    waitSet.dispatch();
    ASSERT_TRUE(handlerExecuted) << "In constructor attached GuardCondition functor was not triggered";
    guard_cond.trigger_value(false);
}

/**
 * Test StatusCondition
 */
TEST_F(Condition, status_condition)
{
    dds::core::cond::StatusCondition reader_status_cond = dds::core::null;

    reader_status_cond = dds::core::cond::StatusCondition(reader);
    reader_status_cond.enabled_statuses(dds::core::status::StatusMask::data_available());

    dds::core::cond::StatusCondition reader_status_cond2(reader, functor_status_cond);

    // Should be the same condition with same mask
    ASSERT_EQ(reader_status_cond2, reader_status_cond) << "StatusConditions are not equal";
    ASSERT_EQ(reader_status_cond2.enabled_statuses(), dds::core::status::StatusMask::data_available()) << "StatusConditions have different masks";

    // Should be different than writer status cond
    dds::core::cond::StatusCondition writer_status_cond(writer);
    ASSERT_NE(writer_status_cond, reader_status_cond) << "Reader StatusCondition does not distinguish itself from Writer StatusCondition";

    // Write a sample
    Space::Type1 sample(1, 2, 3);
    writer << sample;

    // Check if data available and forward to dispatch
    ASSERT_TRUE(reader_status_cond.trigger_value()) << "StatusCondition did not trigger";
    reader_status_cond.dispatch();

    // Add status condition to WaitSet
    dds::core::cond::WaitSet waitset = dds::core::cond::WaitSet();
    waitset.attach_condition(reader_status_cond);

    // Check if successfully attached
    dds::core::cond::WaitSet::ConditionSeq conds = waitset.conditions();
    ASSERT_EQ(conds.size(), 1) << "Number of conditions incorrect";
    ASSERT_EQ(conds[0], reader_status_cond) << "Attached condition is not the expected StatusCondition";
}

/**
 * Test creating status condition on all entities
 */
TEST_F(Condition, status_condition_entity)
{
    dds::core::cond::StatusCondition status_cond = dds::core::null;

    // Participant
    status_cond = dds::core::cond::StatusCondition(participant);
    ASSERT_FALSE(status_cond == dds::core::null) << "StatusCondition object is null after creation on participant";
    ASSERT_EQ(status_cond.entity(), participant) << "Entity (participant) on condition is not equal to condition used in constructor";

    // Publisher
    status_cond = dds::core::cond::StatusCondition(publisher);
    ASSERT_FALSE(status_cond == dds::core::null) << "StatusCondition object is null after creation on publisher";
    ASSERT_EQ(status_cond.entity(), publisher) << "Entity (publisher) on condition is not equal to condition used in constructor";

    // Subscriber
    status_cond = dds::core::cond::StatusCondition(subscriber);
    ASSERT_FALSE(status_cond == dds::core::null) << "StatusCondition object is null after creation on subscriber";
    ASSERT_EQ(status_cond.entity(), subscriber) << "Entity (subscriber) on condition is not equal to condition used in constructor";

    // Reader
    status_cond = dds::core::cond::StatusCondition(reader);
    ASSERT_FALSE(status_cond == dds::core::null) << "StatusCondition object is null after creation on reader";
    ASSERT_EQ(status_cond.entity(), reader) << "Entity (reader) on condition is not equal to condition used in constructor";

    // Writer
    status_cond = dds::core::cond::StatusCondition(writer);
    ASSERT_FALSE(status_cond == dds::core::null) << "StatusCondition object is null after creation on writer";
    ASSERT_EQ(status_cond.entity(), writer) << "Entity (writer) on condition is not equal to condition used in constructor";
}

/**
 * Test GuardCondition
 */
TEST_F(Condition, guard_condition)
{
    dds::core::cond::GuardCondition guard_cond = dds::core::null;
    guard_cond = dds::core::cond::GuardCondition();

    // Test trigger value
    guard_cond.trigger_value(true);
    ASSERT_TRUE(guard_cond.trigger_value());

    guard_cond.trigger_value(false);
    ASSERT_FALSE(guard_cond.trigger_value());

    // Add guard condition to WaitSet
    dds::core::cond::WaitSet waitset = dds::core::cond::WaitSet();
    waitset.attach_condition(guard_cond);

    // Check if successfully attached
    dds::core::cond::WaitSet::ConditionSeq conds = waitset.conditions();
    ASSERT_EQ(conds.size(), 1) << "Number of conditions incorrect";
    ASSERT_EQ(conds[0], guard_cond) << "Attached condition is not the expected GuardCondition";
}

/**
 * Test ReadCondition
 */
TEST_F(Condition, read_condition)
{
    dds::sub::status::DataState state_filter;
    dds::sub::cond::ReadCondition read_cond = dds::core::null;

    // Create ReadCondition with state_filter all
    state_filter = dds::sub::status::DataState::any();
    read_cond = dds::sub::cond::ReadCondition(reader, state_filter);

    // Data sample
    Space::Type1 sample(1, 2, 3);

    // Check state filter
    dds::sub::status::DataState tmp_state_filter1 = read_cond.state_filter();
    ASSERT_TRUE(tmp_state_filter1 == state_filter) << "The state filter is not correct";

    // Create ReadCondition with specific state filter
    dds::sub::status::SampleState ss = dds::sub::status::SampleState::not_read();
    dds::sub::status::ViewState vs = dds::sub::status::ViewState::new_view();
    dds::sub::status::InstanceState is = dds::sub::status::InstanceState::alive();
    
    state_filter = dds::sub::status::DataState(ss, vs, is);
    read_cond = dds::sub::cond::ReadCondition(reader, state_filter);

    // Check state filter
    dds::sub::status::DataState tmp_state_filter2 = read_cond.state_filter();
    ASSERT_TRUE(tmp_state_filter2 == state_filter) << "The state filter is not correct";

    // Check default trigger value
    ASSERT_FALSE(read_cond.trigger_value()) << "The trigger_value is not correct (true)";

    // Write sample and wait for data
    writer << sample;
    wait_for_data(reader);

    // Condition should be triggered
    ASSERT_TRUE(read_cond.trigger_value()) << "The trigger_value is not correct (false)";

    // Check reader for this ReadCondition
    dds::sub::AnyDataReader ar = read_cond.data_reader();
    ASSERT_EQ(ar->get_ddsc_entity(), reader->get_ddsc_entity()) << "The returned reader is incorrect";
}

/**
 * Test read condition functor
 */
TEST_F(Condition, read_condition_functor)
{
    dds::sub::status::DataState state_filter;
    dds::sub::cond::ReadCondition read_cond = dds::core::null;

    // Data sample
    Space::Type1 sample(1, 2, 3);

    // Create functor that check sample
    FunctorReadConditionSample functor_sample(sample);

    // Create ReadCondition with state_filter all
    state_filter = dds::sub::status::DataState::any();
    read_cond = dds::sub::cond::ReadCondition(reader, state_filter, functor_sample);

    // Write sample and wait for data
    writer << sample;
    wait_for_data(reader);

    ASSERT_TRUE(read_cond.trigger_value()) << "The trigger_value is not correct (false)";

    dds::sub::LoanedSamples<Space::Type1> samples = reader.take();
    ASSERT_GT(samples.length(), 0u) << "FAILED";

    // Dispatch to functor
    ASSERT_NO_THROW({
        read_cond.dispatch();
    }) << "Dispatch throws an error";
}

/**
 * Test query condition
 */
TEST_F(Condition, query_condition)
{
    dds::sub::cond::QueryCondition query_cond = dds::core::null;

    // We need a query to create a QueryCondition
    std::vector<std::string> params;
    params.push_back("1");
    dds::sub::Query query = dds::sub::Query(reader, "long_1=%0", params);

    // QueryCondition is currently not supported
    ASSERT_THROW({ 
        query_cond = dds::sub::cond::QueryCondition(query, dds::sub::status::DataState::any());
    }, dds::core::UnsupportedError);
}

/**
 * Test conversion of null condition objects
 */
TEST_F(Condition, conversion_null)
{
    dds::core::cond::StatusCondition status_cond = dds::core::null;
    dds::core::cond::GuardCondition guard_cond = dds::core::null;
    dds::sub::cond::ReadCondition read_cond = dds::core::null;
    // dds::sub::cond::QueryCondition query_cond = dds::core::null;

    // StatusCondition
    {
        // null conversions by construction
        dds::core::cond::Condition cond(status_cond);
        dds::core::cond::StatusCondition tmpCond(cond);
        ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by construction failed for StatusCondition";
    }
    {
        // null conversions by assignment
        dds::core::cond::Condition cond = dds::core::null;
        dds::core::cond::StatusCondition tmpCond = dds::core::null;
        cond = status_cond;
        tmpCond = cond;
        ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by assignment failed for StatusCondition";
    }
    {
        // null implicit conversions
        dds::core::cond::StatusCondition tmp1 = dds::core::null;
        ConditionStore store(status_cond);
        dds::core::cond::StatusCondition tmp2(store.get());
        tmp1 = store.get();
        dds::core::cond::Condition cond = store.get();
        ASSERT_TRUE(tmp1 == status_cond
                 && tmp2 == status_cond
                 && cond == status_cond) << "Implicit null conversion failed for StatusCondition";
    }

    // Guard condition
    {
        // null conversions by construction
        dds::core::cond::Condition cond(guard_cond);
        dds::core::cond::GuardCondition tmpCond(cond);
        ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by construction failed for GuardCondition";
    }
    {
        // null conversions by assignment
        dds::core::cond::Condition cond = dds::core::null;
        dds::core::cond::GuardCondition tmpCond = dds::core::null;
        cond = guard_cond;
        tmpCond = cond;
        ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by assignment failed for GuardCondition";
    }
    {
        // null implicit conversions
        dds::core::cond::GuardCondition tmp1 = dds::core::null;
        ConditionStore store(guard_cond);
        dds::core::cond::GuardCondition tmp2(store.get());
        tmp1 = store.get();
        dds::core::cond::Condition cond = store.get();
        ASSERT_TRUE(tmp1 == guard_cond
                 && tmp2 == guard_cond
                 && cond == guard_cond) << "Implicit null conversion failed for GuardCondition";
    }

    // Read condition
    {
        // null conversions by construction
        dds::core::cond::Condition cond(read_cond);
        dds::sub::cond::ReadCondition tmpCond(cond);
        ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by construction failed for ReadCondition";
    }
    {
        // null conversions by assignment
        dds::core::cond::Condition cond = dds::core::null;
        dds::sub::cond::ReadCondition tmpCond = dds::core::null;
        cond = read_cond;
        tmpCond = cond;
        ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by assignment failed for ReadCondition";
    }
    {
        // null implicit conversions
        dds::sub::cond::ReadCondition tmp1 = dds::core::null;
        ConditionStore store(read_cond);
        dds::sub::cond::ReadCondition tmp2(store.get());
        tmp1 = store.get();
        dds::core::cond::Condition cond = store.get();
        ASSERT_TRUE(tmp1 == read_cond
                 && tmp2 == read_cond
                 && cond == read_cond) << "Implicit null conversion failed for ReadCondition";
    }

    // Query condition (not supported)
    // {
    //     // null conversions by construction
    //     dds::core::cond::Condition cond(query_cond);
    //     dds::sub::cond::QueryCondition tmpCond(cond);
    //     ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by construction failed for QueryCondition";
    // }
    // {
    //     // null conversions by assignment
    //     dds::core::cond::Condition cond = dds::core::null;
    //     dds::sub::cond::QueryCondition tmpCond = dds::core::null;
    //     cond = query_cond;
    //     tmpCond = cond;
    //     ASSERT_EQ(tmpCond, dds::core::null) << "Null conversion by assignment failed for QueryCondition";
    // }
    // {
    //     // null implicit conversions
    //     dds::sub::cond::QueryCondition tmp1 = dds::core::null;
    //     ConditionStore store(query_cond);
    //     dds::sub::cond::QueryCondition tmp2(store.get());
    //     tmp1 = store.get();
    //     dds::core::cond::Condition cond = store.get();
    //     ASSERT_TRUE(tmp1 == query_cond
    //              && tmp2 == query_cond
    //              && cond == query_cond) << "Implicit null conversion failed for QueryCondition";
    // }
}

/**
 * Test conversion of condition objects
 */
TEST_F(Condition, conversion)
{
    dds::core::cond::StatusCondition status_cond = dds::core::null;
    dds::core::cond::GuardCondition guard_cond = dds::core::null;
    dds::sub::cond::ReadCondition read_cond = dds::core::null;
    // dds::sub::cond::QueryCondition query_cond = dds::core::null;

    // Try to create the conditions with the ConditionFunctor, just to see if it compiles
    status_cond = dds::core::cond::StatusCondition(participant, functor_cond);
    guard_cond = dds::core::cond::GuardCondition(functor_cond);
    // query_cond = dds::sub::cond::QueryCondition(query, dds::sub::status::DataState::any(), functor_cond);
    read_cond = dds::sub::cond::ReadCondition(reader, dds::sub::status::DataState::any(), functor_cond);
    ASSERT_FALSE(status_cond == dds::core::null
            || guard_cond == dds::core::null
    //        || query_cond == dds::core::null
            || read_cond == dds::core::null);

    // Try to create the conditions with their specific functors
    status_cond = dds::core::cond::StatusCondition(participant, functor_status_cond);
    guard_cond = dds::core::cond::GuardCondition(functor_guard_cond);
    // query_cond = dds::sub::cond::QueryCondition(query, dds::sub::status::DataState::any(), functor_query_cond);
    read_cond = dds::sub::cond::ReadCondition(reader, dds::sub::status::DataState::any(), functor_read_cond);

    ASSERT_FALSE(status_cond == dds::core::null
            || guard_cond == dds::core::null
    //        || query_cond == dds::core::null
            || read_cond == dds::core::null);

    // StatusCondition
    {
        // conversions by construction
        dds::core::cond::Condition cond(status_cond);
        dds::core::cond::StatusCondition tmpCond(cond);
        ASSERT_EQ(tmpCond, status_cond) << "Conversion by construction failed for StatusCondition";
    }
    {
        // conversions by assignment
        dds::core::cond::Condition cond = dds::core::null;
        dds::core::cond::StatusCondition tmpCond = dds::core::null;
        cond = status_cond;
        tmpCond = cond;
        ASSERT_EQ(tmpCond, status_cond) << "Conversion by assignment failed for StatusCondition";
    }
    {
        // implicit conversions
        dds::core::cond::StatusCondition tmp1 = dds::core::null;
        ConditionStore store(status_cond);
        dds::core::cond::StatusCondition tmp2(store.get());
        tmp1 = store.get();
        dds::core::cond::Condition cond = store.get();
        ASSERT_TRUE(tmp1 == status_cond
                 && tmp2 == status_cond
                 && cond == status_cond) << "Implicit conversion failed for StatusCondition";
    }

    // Guard condition
    {
        // conversions by construction
        dds::core::cond::Condition cond(guard_cond);
        dds::core::cond::GuardCondition tmpCond(cond);
        ASSERT_EQ(tmpCond, guard_cond) << "Conversion by construction failed for GuardCondition";
    }
    {
        // conversions by assignment
        dds::core::cond::Condition cond = dds::core::null;
        dds::core::cond::GuardCondition tmpCond = dds::core::null;
        cond = guard_cond;
        tmpCond = cond;
        ASSERT_EQ(tmpCond, guard_cond) << "Conversion by assignment failed for GuardCondition";
    }
    {
        // implicit conversions
        dds::core::cond::GuardCondition tmp1 = dds::core::null;
        ConditionStore store(guard_cond);
        dds::core::cond::GuardCondition tmp2(store.get());
        tmp1 = store.get();
        dds::core::cond::Condition cond = store.get();
        ASSERT_TRUE(tmp1 == guard_cond
                 && tmp2 == guard_cond
                 && cond == guard_cond) << "Implicit conversion failed for GuardCondition";
    }

    // Read condition
    {
        // conversions by construction
        dds::core::cond::Condition cond(read_cond);
        dds::sub::cond::ReadCondition tmpCond(cond);
        ASSERT_EQ(tmpCond, read_cond) << "Conversion by construction failed for ReadCondition";
    }
    {
        // conversions by assignment
        dds::core::cond::Condition cond = dds::core::null;
        dds::sub::cond::ReadCondition tmpCond = dds::core::null;
        cond = read_cond;
        tmpCond = cond;
        ASSERT_EQ(tmpCond, read_cond) << "Conversion by assignment failed for ReadCondition";
    }
    {
        // implicit conversions
        dds::sub::cond::ReadCondition tmp1 = dds::core::null;
        ConditionStore store(read_cond);
        dds::sub::cond::ReadCondition tmp2(store.get());
        tmp1 = store.get();
        dds::core::cond::Condition cond = store.get();
        ASSERT_TRUE(tmp1 == read_cond
                 && tmp2 == read_cond
                 && cond == read_cond) << "Implicit conversion failed for ReadCondition";
    }

    // Query condition
    // {
    //     // conversions by construction
    //     dds::core::cond::Condition cond(query_cond);
    //     dds::sub::cond::QueryCondition tmpCond(cond);
    //     ASSERT_EQ(tmpCond, query_cond) << "Conversion by construction failed for QueryCondition";
    // }
    // {
    //     // conversions by assignment
    //     dds::core::cond::Condition cond = dds::core::null;
    //     dds::sub::cond::QueryCondition tmpCond = dds::core::null;
    //     cond = query_cond;
    //     tmpCond = cond;
    //     ASSERT_EQ(tmpCond, query_cond) << "Conversion by assignment failed for QueryCondition";
    // }
    // {
    //     // implicit conversions
    //     dds::sub::cond::QueryCondition tmp1 = dds::core::null;
    //     ConditionStore store(query_cond);
    //     dds::sub::cond::QueryCondition tmp2(store.get());
    //     tmp1 = store.get();
    //     dds::core::cond::Condition cond = store.get();
    //     ASSERT_TRUE(tmp1 == query_cond
    //              && tmp2 == query_cond
    //              && cond == query_cond) << "Implicit conversion failed for QueryCondition";
    // }
}

/**
 * Test invalid conversion
 */
TEST_F(Condition, conversion_invalid)
{
    dds::core::cond::StatusCondition status_cond = dds::core::null;
    dds::core::cond::GuardCondition guard_cond = dds::core::null;
    dds::sub::cond::ReadCondition read_cond = dds::core::null;
    // dds::sub::cond::QueryCondition query_cond = dds::core::null;

    status_cond = dds::core::cond::StatusCondition(participant, functor_cond);
    guard_cond = dds::core::cond::GuardCondition(functor_cond);
    // query_cond = dds::sub::cond::QueryCondition(query, dds::sub::status::DataState::any(), functor_cond);
    read_cond = dds::sub::cond::ReadCondition(reader, dds::sub::status::DataState::any(), functor_cond);

    // guard to status
    ASSERT_THROW({
        dds::core::cond::Condition cond = guard_cond;
        dds::core::cond::StatusCondition tmp = cond;
    }, dds::core::IllegalOperationError) << "Conversion from GuardCondition to StatusCondition did not throw an exception";

    // guard to read
    ASSERT_THROW({
        dds::core::cond::Condition cond = guard_cond;
        dds::sub::cond::ReadCondition tmp = cond;
    }, dds::core::IllegalOperationError) << "Conversion from GuardCondition to ReadCondition did not throw an exception";

    // query to status
    // ASSERT_THROW({
    //     dds::core::cond::Condition cond = query_cond;
    //     dds::core::cond::StatusCondition tmp = cond;
    // }, dds::core::IllegalOperationError) << "Conversion from QueryCondition to StatusCondition did not throw an exception";

    // query to guard
    // ASSERT_THROW({
    //     dds::core::cond::Condition cond = query_cond;
    //     dds::core::cond::GuardCondition tmp = cond;
    // }, dds::core::IllegalOperationError) << "Conversion from QueryCondition to GuardCondition did not throw an exception";

    // read to status
    ASSERT_THROW({
        dds::core::cond::Condition cond = read_cond;
        dds::core::cond::StatusCondition tmp = cond;
    }, dds::core::IllegalOperationError) << "Conversion from ReadCondition to StatusCondition did not throw an exception";

    // read to guard
    ASSERT_THROW({
        dds::core::cond::Condition cond = read_cond;
        dds::core::cond::GuardCondition tmp = cond;
    }, dds::core::IllegalOperationError) << "Conversion from ReadCondition to GuardCondition did not throw an exception";

    // status to read
    ASSERT_THROW({
        dds::core::cond::Condition cond = status_cond;
        dds::sub::cond::ReadCondition tmp = cond;
    }, dds::core::IllegalOperationError) << "Conversion from StatusCondition to ReadCondition did not throw an exception";

    // status to query
    // ASSERT_THROW({
    //     dds::core::cond::Condition cond = status_cond;
    //     dds::sub::cond::QueryCondition tmp = cond;
    // }, dds::core::IllegalOperationError) << "Conversion from StatusCondition to QueryCondition did not throw an exception";
}
