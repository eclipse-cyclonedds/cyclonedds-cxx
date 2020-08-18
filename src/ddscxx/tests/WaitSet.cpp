/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include "dds/dds.hpp"
#include "dds/ddscxx/test.h"
#include "Space_DCPS.hpp"

#include "dds/ddsrt/threads.h"
#include "dds/ddsrt/sync.h"


#define TA_ATTACH_REMOVE_CONDITION "attach_remove_condition"
#define TA_ATTACH_CLOSE_READER     "close_reader"
#define TA_ADD_GUARD_CONDITION     "add_guard_condition"
#define TA_WAITSET_DESTRUCTOR      "waitset_destructor"


class StatusCondHandler
{
public:
    StatusCondHandler(bool& statusHandlerExecuted)
        : statusHandlerExecuted(statusHandlerExecuted) { }

    void operator() (dds::core::Entity&)
    {
        statusHandlerExecuted = true;
    }

    void operator() (dds::core::cond::Condition&)
    {
        statusHandlerExecuted = true;
    }

private:
    bool& statusHandlerExecuted;
};

class GuardCondHandler
{
public:
    GuardCondHandler(bool& guardHandlerExecuted)
    : guardHandlerExecuted(guardHandlerExecuted) { }

    void operator() ()
    {
        guardHandlerExecuted = true;
    }

    void operator() (dds::core::cond::Condition&)
    {
        guardHandlerExecuted = true;
    }

private:
    bool& guardHandlerExecuted;
};

class ReadCondHandler
{
public:
    ReadCondHandler(bool& readHandlerExecuted)
    : readHandlerExecuted(readHandlerExecuted) { }

    void operator() (dds::sub::DataReader<Space::Type1>&)
    {
        readHandlerExecuted = true;
    }

    void operator() (dds::core::cond::Condition&)
    {
        readHandlerExecuted = true;
    }

private:
    bool& readHandlerExecuted;
};

struct test_semaphore {
    ddsrt_cond_t cond;
    ddsrt_mutex_t mutex;
    int value;
};

struct writer_thread_args {
    dds::pub::DataWriter<Space::Type1> * writer;
    dds_duration_t delay;
};

struct guard_thread_args {
    dds::core::cond::GuardCondition * guard;
    dds_duration_t delay;
};

struct action_thread_args {
    std::string action;
    dds::core::cond::WaitSet * waitSet;
    dds::core::cond::GuardCondition * guard;
    dds::core::cond::StatusCondition * readerStatus;
    dds::sub::DataReader<Space::Type1> * reader;
    dds::pub::DataWriter<Space::Type1> * writer;
    test_semaphore * semStart;
    test_semaphore * semReady;
    bool result;
    std::string message;
};

void test_sem_init(test_semaphore *sem, int value)
{
    ddsrt_mutex_init(&sem->mutex);
    ddsrt_cond_init(&sem->cond);
    sem->value = value;
}

void test_sem_destroy(test_semaphore *sem)
{
    ddsrt_cond_destroy(&sem->cond);
    ddsrt_mutex_destroy(&sem->mutex);
    sem->value = 0;
}

void test_sem_lock(test_semaphore *sem)
{
    ddsrt_mutex_lock(&sem->mutex);
}

bool test_sem_wait(test_semaphore *sem, const dds_duration_t timeout)
{
    bool result = false;
    sem->value -= 1;

    result = ddsrt_cond_waitfor(&sem->cond, &sem->mutex, timeout);

    ddsrt_mutex_unlock(&sem->mutex);

    return result;
}

void test_sem_post(test_semaphore *sem)
{
    ddsrt_mutex_lock(&sem->mutex);
    sem->value += 1;
    if (sem->value >= 0) {
        ddsrt_cond_broadcast(&sem->cond);
    }
    ddsrt_mutex_unlock(&sem->mutex);
}

static uint32_t guard_thread(void *arg)
{
    guard_thread_args * args = (guard_thread_args *)arg;

    try {
        dds_sleepfor(args->delay);
        args->guard->trigger_value(true);
    } catch (const dds::core::Exception& e) {
        std::cout << "guard trigger fails: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "guard trigger fails" << std::endl;
    }

    return 0;
}

static uint32_t writer_thread(void *arg)
{
    writer_thread_args * args = (writer_thread_args *)arg;

    try {
        dds_sleepfor(args->delay);
        dds::pub::DataWriter<Space::Type1> * writer = args->writer;
        Space::Type1 testData(1, 2, 3);
        (*writer) << testData;
    } catch (const dds::core::Exception& e) {
        std::cout << "write fails: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "write fails" << std::endl;
    }

    return 0;
}

static uint32_t test_action_thread(void *arg)
{
    action_thread_args * args = (action_thread_args *)arg;

    try
    {
        if (args->action == TA_ATTACH_REMOVE_CONDITION) {
            // Add statuscondition
            dds_sleepfor(DDS_MSECS(100));
            *args->waitSet += *args->readerStatus;

            // Remove guard condition
            dds_sleepfor(DDS_MSECS(100));
            *args->waitSet -= *args->guard;

            Space::Type1 testData(1, 2, 3);
            *args->writer << testData;

        } else if (args->action == TA_ATTACH_CLOSE_READER) {
            dds_sleepfor(DDS_MSECS(200));
            args->reader->close();
            dds_sleepfor(DDS_MSECS(200));
            args->guard->trigger_value(true);

        } else if (args->action == TA_ADD_GUARD_CONDITION) {
            dds::core::cond::GuardCondition guard1;
            dds_sleepfor(DDS_MSECS(200));
            *args->waitSet += guard1;
            *args->waitSet -= *args->guard;

            dds_sleepfor(DDS_MSECS(100));
            args->guard->trigger_value(true);

            dds_sleepfor(DDS_MSECS(100));
            guard1.trigger_value(true);

        } else if (args->action == TA_WAITSET_DESTRUCTOR) {
            test_sem_post(args->semStart);
            dds::core::cond::WaitSet ws;
            dds::core::cond::GuardCondition guardCond;
            dds::core::cond::StatusCondition statusCond(*args->reader);
            dds::sub::status::DataState anyDataState;
            dds::sub::cond::ReadCondition readCond(*args->reader, anyDataState);

            // TODO: add query condition
            // std::vector<std::string> params;
            // params.push_back("0");
            // dds::sub::Query query(*args->reader, "long_1 > %0", params);
            // dds::sub::cond::QueryCondition queryCond(query, anyDataState);

            ws += guardCond;
            ws += statusCond;
            ws += readCond;
            // ws += queryCond;

            test_sem_post(args->semReady);
        }
    }
    catch (const dds::core::Exception& e) {
        args->result = false;
        args->message = e.what();
    }

    return 0;
}


/**
 * Fixture for the tests
 */
class ddscxx_WaitSet : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::pub::Publisher publisher;
    dds::sub::Subscriber subscriber;
    dds::topic::Topic<Space::Type1> topic;
    dds::pub::DataWriter<Space::Type1> writer;
    dds::sub::DataReader<Space::Type1> reader;

    dds::core::cond::WaitSet waitSet;
    dds::sub::cond::ReadCondition readerCond;
    dds::core::cond::StatusCondition readerStatus;
    dds::core::cond::GuardCondition guard;

    dds::core::status::StatusMask statusMask;

    bool statusHandlerExecuted;
    bool guardHandlerExecuted;
    StatusCondHandler statusCondHandler;
    GuardCondHandler guardCondHandler;

    writer_thread_args writerThreadArgs;
    guard_thread_args guardThreadArgs;
    action_thread_args actionThreadArgs;
    ddsrt_thread_t threadId;
    ddsrt_threadattr_t threadAttr;

    test_semaphore start_sem;
    test_semaphore ready_sem;

    ddscxx_WaitSet() :
        participant(dds::core::null),
        publisher(dds::core::null),
        subscriber(dds::core::null),
        topic(dds::core::null),
        writer(dds::core::null),
        reader(dds::core::null),
        waitSet(dds::core::null),
        readerCond(dds::core::null),
        readerStatus(dds::core::null),
        statusHandlerExecuted(false),
        guardHandlerExecuted(false),
        statusCondHandler(statusHandlerExecuted),
        guardCondHandler(guardHandlerExecuted)
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

        this->topic = dds::topic::Topic<Space::Type1>(this->participant, "waitset_test_topic");
        ASSERT_NE(this->topic, dds::core::null);

        this->reader = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic);
        ASSERT_NE(this->reader, dds::core::null);

        this->writer = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic);
        ASSERT_NE(this->writer, dds::core::null);

        // Init statuscondition
        readerStatus = dds::core::cond::StatusCondition(reader, statusCondHandler);
        statusMask << dds::core::status::StatusMask::data_available();
        readerStatus.enabled_statuses(statusMask);

        // Init guardcondition
        guard.handler(guardCondHandler);

        // Semaphore init
        test_sem_init(&start_sem, 0);
        test_sem_init(&ready_sem, 0);

        // Thread init
        writerThreadArgs.writer = &writer;

        guardThreadArgs.guard = &guard;

        actionThreadArgs.result = true;
        actionThreadArgs.waitSet = &waitSet;
        actionThreadArgs.guard = &guard;
        actionThreadArgs.readerStatus = &readerStatus;
        actionThreadArgs.reader = &reader;
        actionThreadArgs.writer = &writer;
        actionThreadArgs.semStart = &start_sem;
        actionThreadArgs.semReady = &ready_sem;

        ddsrt_threadattr_init(&threadAttr);

    }

    void TearDown()
    {
        this->reader = dds::core::null;
        this->writer = dds::core::null;
        this->topic = dds::core::null;
        this->subscriber = dds::core::null;
        this->publisher = dds::core::null;
        this->participant = dds::core::null;

        test_sem_destroy(&start_sem);
        test_sem_destroy(&ready_sem);
    }
};



/**
 * Test null assignment for WaitSets
 */
DDSCXX_TEST_F(ddscxx_WaitSet, null)
{
    dds::core::cond::WaitSet waitSet1 = dds::core::null;
    dds::core::cond::WaitSet waitSet2(dds::core::null);
    ASSERT_EQ(waitSet1, dds::core::null);
    ASSERT_EQ(waitSet2, dds::core::null);
}

/**
 * Test creating WaitSet
 */
DDSCXX_TEST_F(ddscxx_WaitSet, create)
{
    dds::core::cond::WaitSet waitSet1 = dds::core::cond::WaitSet();
    dds::core::cond::WaitSet waitSet2 = dds::core::cond::WaitSet();
    ASSERT_NE(waitSet1, dds::core::null);
    ASSERT_NE(waitSet2, dds::core::null);
    ASSERT_NE(waitSet1, waitSet2);
}

/**
 * Test attach condition to WaitSet
 */
DDSCXX_TEST_F(ddscxx_WaitSet, attach_conditions)
{
    waitSet = dds::core::cond::WaitSet();
    ASSERT_NE(waitSet, dds::core::null);

    // Add statuscondition and check if in list
    waitSet += readerStatus;
    ASSERT_EQ(waitSet.conditions().size(), 1);

    // Remove statuscondition and check if removed from list
    waitSet -= readerStatus;
    ASSERT_EQ(waitSet.conditions().size(), 0);

    // Add status and wait condition and check condition count
    waitSet += readerStatus;
    waitSet += guard;
    ASSERT_EQ(waitSet.conditions().size(), 2);

    // Remove conditions and check count
    waitSet -= readerStatus;
    ASSERT_EQ(waitSet.conditions().size(), 1);
    waitSet -= guard;
    ASSERT_EQ(waitSet.conditions().size(), 0);
}

/**
 * Test wait() timeout
 */
DDSCXX_TEST_F(ddscxx_WaitSet, wait_timeout)
{
    waitSet = dds::core::cond::WaitSet();
    waitSet += readerStatus;

    ASSERT_THROW({
        dds::core::Duration waitTimeout = dds::core::Duration::from_millisecs(100);
        dds::core::cond::WaitSet::ConditionSeq conditionList = waitSet.wait(waitTimeout);
        ASSERT_EQ(conditionList.size(), 0);
    }, dds::core::TimeoutError);

    waitSet -= readerStatus;
}

/**
 * Test waiting for StatusCondition
 */
DDSCXX_TEST_F(ddscxx_WaitSet, wait_reader_status)
{
    waitSet = dds::core::cond::WaitSet();
    waitSet += readerStatus;

    // write some data
    writerThreadArgs.delay = DDS_MSECS(100);
    ddsrt_thread_create(&threadId, "writer_thread",
                &threadAttr, writer_thread, (void*)&writerThreadArgs);

    // wait for status condition
    ASSERT_NO_THROW({
        dds::core::Duration waitTimeout = dds::core::Duration::from_millisecs(500);
        dds::core::cond::WaitSet::ConditionSeq conditionList = waitSet.wait(waitTimeout);

        ASSERT_EQ(conditionList.size(), 1);
        ASSERT_EQ(conditionList[0], readerStatus);
    });

    reader.take();
    waitSet -= readerStatus;

    ddsrt_thread_join(threadId, NULL);
}

/**
 * Test timeout when waiting for StatusCondition
 */
DDSCXX_TEST_F(ddscxx_WaitSet, wait_reader_timeout)
{
    dds::core::cond::WaitSet::ConditionSeq conditionList;
    dds::core::Duration waitTimeout;

    waitSet = dds::core::cond::WaitSet();
    waitSet += readerStatus;

    // Write data
    writerThreadArgs.delay = DDS_MSECS(100);
    ddsrt_thread_create(&threadId, "writer_thread",
                &threadAttr, writer_thread, (void*)&writerThreadArgs);

    // Wait and expect being triggered
    ASSERT_NO_THROW({
        waitTimeout = dds::core::Duration::from_millisecs(500);
        waitSet.wait(conditionList, waitTimeout);
    });
    ASSERT_EQ(conditionList.size(), 1);
    ASSERT_EQ(conditionList[0], readerStatus);

    // Take the data
    dds_sleepfor(DDS_MSECS(100));
    reader.take();
    dds_sleepfor(DDS_MSECS(100));

    // Wait and expect time-out
    ASSERT_THROW({
        waitTimeout = dds::core::Duration::from_millisecs(500);
        waitSet.wait(conditionList, waitTimeout);
    }, dds::core::TimeoutError);

    waitSet -= readerStatus;

    ddsrt_thread_join(threadId, NULL);
}

/**
 * Test GuardCondition trigger during wait
 */
DDSCXX_TEST_F(ddscxx_WaitSet, guard_trigger_during_wait)
{
    dds::core::cond::WaitSet::ConditionSeq conditionList;
    dds::core::Duration waitTimeout;

    waitSet = dds::core::cond::WaitSet();
    waitSet += guard;

    // Trigger the guard condition during wait
    guardThreadArgs.delay = DDS_MSECS(100);
    ddsrt_thread_create(&threadId, "guard_trigger_thread",
                &threadAttr, guard_thread, (void*)&guardThreadArgs);

    // wait for the GuardCondition to trigger
    ASSERT_NO_THROW({
        waitTimeout = dds::core::Duration::from_millisecs(500);
        conditionList = waitSet.wait(waitTimeout);
    });
    ASSERT_EQ(conditionList.size(), 1);
    ASSERT_EQ(conditionList[0], guard);

    // Clean-up
    guard.trigger_value(false);
    waitSet -= guard;

    ddsrt_thread_join(threadId, NULL);
}

/**
 * Test GuardCondition trigger before wait
 */
DDSCXX_TEST_F(ddscxx_WaitSet, guard_trigger_before_wait)
{
    dds::core::cond::WaitSet::ConditionSeq conditionList;
    dds::core::Duration waitTimeout;

    waitSet = dds::core::cond::WaitSet();
    waitSet += guard;

    // Trigger the guard condition before wait
    guard.trigger_value(true);

    // Wait for the GuardCondition
    ASSERT_NO_THROW({
        waitTimeout = dds::core::Duration::from_millisecs(500);
        conditionList = waitSet.wait(waitTimeout);
    });
    ASSERT_EQ(conditionList.size(), 1);
    ASSERT_EQ(conditionList[0], guard);

    // Clean-up
    guard.trigger_value(false);
    waitSet -= guard;
}

/**
 * Test adding multiple conditions to WaitSet
 */
DDSCXX_TEST_F(ddscxx_WaitSet, multiple_conditions)
{
    // Create read condition
    dds::sub::status::DataState anyDataState;
    dds::sub::cond::ReadCondition readCond(reader, anyDataState);

    // Add read, status and guard condition to waitset
    waitSet = dds::core::cond::WaitSet();
    waitSet += readCond;
    waitSet += readerStatus;
    waitSet += guard;
    ASSERT_EQ(waitSet.conditions().size(), 3);

    // Write data to trigger read and status condition
    Space::Type1 testData(1, 2, 3);
    writer << testData;
    dds_sleepfor(DDS_MSECS(100));

    // Trigger guard condition
    guard.trigger_value(true);

    // Wait for conditions
    dds::core::cond::WaitSet::ConditionSeq conditionList;
    waitSet.wait(conditionList);

    ASSERT_EQ(conditionList.size(), 3);

    // Check if all conditions in resulting list
    bool readSeen = false;
    bool statusSeen = false;
    bool guardSeen = false;
    for (dds::core::cond::WaitSet::ConditionSeq::iterator i = conditionList.begin(); i < conditionList.end(); i++) {
        if (*i == readCond) {
            readSeen = true;
        } else if(*i == readerStatus) {
            statusSeen = true;
        } else if(*i == guard) {
            guardSeen = true;
        }
    }
    ASSERT_TRUE(readSeen);
    ASSERT_TRUE(statusSeen);
    ASSERT_TRUE(guardSeen);

    // Clean-up
    guard.trigger_value(false);
    reader.take();
    waitSet -= readCond;
    waitSet -= readerStatus;
    waitSet -= guard;
}

/**
 * Add multiple conditions to a WaitSet and check if all handlers functors are
 * executed with a dispatch
 */
DDSCXX_TEST_F(ddscxx_WaitSet, multiple_conditions_handlers)
{
    bool readHandlerExecuted = false;
    ReadCondHandler readCondHandler(readHandlerExecuted);
    dds::sub::status::DataState anyDataState;
    dds::sub::cond::ReadCondition readCond(reader, anyDataState, readCondHandler);

    // Add read, status and guard condition to waitset
    waitSet = dds::core::cond::WaitSet();
    waitSet += readCond;
    waitSet += readerStatus;
    waitSet += guard;
    ASSERT_EQ(waitSet.conditions().size(), 3);

    // Write data
    Space::Type1 testData(1, 2, 3);
    writer << testData;
    dds_sleepfor(DDS_MSECS(100));

    // Trigger guard condition
    guard.trigger_value(true);

    // dispatch handler functors
    waitSet.dispatch();

    ASSERT_TRUE(readHandlerExecuted);
    ASSERT_TRUE(statusHandlerExecuted);
    ASSERT_TRUE(guardHandlerExecuted);

    // Clean-up
    reader.take();
    waitSet -= readCond;
    waitSet -= readerStatus;
    waitSet -= guard;
}

/**
 * Test the time-out for WaitSet dispatch function
 */
DDSCXX_TEST_F(ddscxx_WaitSet, dispatch_timeout)
{
    waitSet = dds::core::cond::WaitSet();
    waitSet += readerStatus;

    ASSERT_THROW({
        dds::core::Duration timeout = dds::core::Duration::from_millisecs(100);
        waitSet.dispatch(timeout);
    }, dds::core::TimeoutError) << "WaitSet did not throw TimeoutError";
}


/**
 * Check the same condition can be added more than once
 */
DDSCXX_TEST_F(ddscxx_WaitSet, multiple_conditions_same)
{
    waitSet = dds::core::cond::WaitSet();
    waitSet += guard;
    ASSERT_NO_THROW({
        waitSet += guard;
    });
    waitSet += readerStatus;
    ASSERT_NO_THROW({
        waitSet += readerStatus;
    });

    ASSERT_EQ(waitSet.conditions().size(), 2);

    waitSet -= guard;
    waitSet -= readerStatus;

    ASSERT_EQ(waitSet.conditions().size(), 0);
}

/**
 * Check that a remove of a not attached condition returns false
 */
DDSCXX_TEST_F(ddscxx_WaitSet, detach_condition_nonexisting)
{
    bool result = false;
    waitSet = dds::core::cond::WaitSet();

    ASSERT_NO_THROW({
        result = waitSet.detach_condition(guard);
    });

    ASSERT_FALSE(result) << "Detach did not return false";
}

/**
 * Check that it is possible to remove and attach conditions while waiting
 */
DDSCXX_TEST_F(ddscxx_WaitSet, attach_detach_during_wait)
{
    waitSet = dds::core::cond::WaitSet();

    // Add guard condition
    guard.trigger_value(false);
    waitSet += guard;

    actionThreadArgs.action = TA_ATTACH_REMOVE_CONDITION;
    ddsrt_thread_create(&threadId, "test_action_thread",
                &threadAttr, test_action_thread, (void*)&actionThreadArgs);

    dds::core::Duration waitTimeout (1, 0);
    dds::core::cond::WaitSet::ConditionSeq conditionList = waitSet.wait(waitTimeout);

    ASSERT_EQ(conditionList.size(), 1) << "Incorrect number of triggered conditions";
    ASSERT_EQ(conditionList[0], readerStatus) << "Wrong condition returned";
    ASSERT_TRUE(actionThreadArgs.result) << actionThreadArgs.message;

    // Clean-up
    reader.take();

    // Remove conditions from waitset
    conditionList = waitSet.conditions();
    dds::core::cond::WaitSet::ConditionSeq::iterator it;
    for (it = conditionList.begin(); it != conditionList.end(); ++it) {
        waitSet -= *it;
    }

    ddsrt_thread_join(threadId, NULL);
}


/**
 * Remove and add guard conditions while waiting
 */
DDSCXX_TEST_F(ddscxx_WaitSet, attach_detach_guard_during_wait)
{
    waitSet = dds::core::cond::WaitSet();

    // Add guard condition
    guard.trigger_value(false);
    waitSet += guard;

    actionThreadArgs.action = TA_ADD_GUARD_CONDITION;
    ddsrt_thread_create(&threadId, "test_action_thread",
                &threadAttr, test_action_thread, (void*)&actionThreadArgs);

    dds::core::Duration waitTimeout (1, 0);
    dds::core::cond::WaitSet::ConditionSeq conditionList = waitSet.wait(waitTimeout);

    ASSERT_EQ(conditionList.size(), 1) << "Incorrect number of triggered conditions";
    ASSERT_NE(conditionList[0], guard) << "Wrong condition returned";
    ASSERT_TRUE(actionThreadArgs.result) << actionThreadArgs.message;

    // Clean-up
    reader.take();

    // Remove conditions from waitset
    conditionList = waitSet.conditions();
    dds::core::cond::WaitSet::ConditionSeq::iterator it;
    for (it = conditionList.begin(); it != conditionList.end(); ++it) {
        waitSet -= *it;
    }

    ddsrt_thread_join(threadId, NULL);
}


/**
 * Check if the waitset destructor does not deadlock
 */
DDSCXX_TEST_F(ddscxx_WaitSet, destructor)
{
    dds_duration_t timeout = DDS_MSECS(1000);
    bool result;

    test_sem_lock(&start_sem);
    test_sem_lock(&ready_sem);

    actionThreadArgs.action = TA_WAITSET_DESTRUCTOR;
    ddsrt_thread_create(&threadId, "test_action_thread",
                &threadAttr, test_action_thread, (void*)&actionThreadArgs);

    result = test_sem_wait(&start_sem, timeout);
    ASSERT_TRUE(result) << "Failed to start test thread";

    result = test_sem_wait(&ready_sem, timeout);
    ASSERT_TRUE(result) << "Waitset destuction probably deadlocked";

    ddsrt_thread_join(threadId, NULL);
}

/**
 * Check that it is possible to remove and attach multiple conditions while waiting
 *
 * TODO: disabled because the conditions are currently not removed from the waitset
 */
DDSCXX_TEST_F(ddscxx_WaitSet, DISABLED_attach_detach_multiple_during_wait)
{
    reader.take();
    waitSet = dds::core::cond::WaitSet();

    // Add status condition
    waitSet += readerStatus;

    // Attach guard condition
    guard.trigger_value(false);
    waitSet += guard;

    // Attach read condition
    dds::sub::status::DataState anyDataState;
    dds::sub::cond::ReadCondition readCond(reader, anyDataState);
    waitSet += readCond;

    // Attach query condition
    std::vector<std::string> params;
    params.push_back("0");
    dds::sub::Query query(reader, "long_1 > %0", params);
    dds::sub::cond::QueryCondition queryCond(query, anyDataState);
    waitSet += queryCond;

    // Create thread to close reader, which should remove read/query/status
    // conditions from the WaitSet. Next the thread triggers the guard condition
    actionThreadArgs.action = TA_ATTACH_CLOSE_READER;
    ddsrt_thread_create(&threadId, "test_action_thread",
                &threadAttr, test_action_thread, (void*)&actionThreadArgs);

    // Wait
    dds::core::Duration waitTimeout(1, 0);
    dds::core::cond::WaitSet::ConditionSeq conditionList = waitSet.wait(waitTimeout);

    // Check if correct condition triggered
    ASSERT_EQ(conditionList.size(), 1) << "Incorrect number of triggered conditions";
    ASSERT_EQ(conditionList[0], guard) << "The GuardCondition was not triggered";

    // Check if conditions are removed
    conditionList = waitSet.conditions();
    ASSERT_EQ(conditionList.size(), 1) << "The status-, read- and query-conditions are not removed";

    ASSERT_TRUE(actionThreadArgs.result) << actionThreadArgs.message;

    // Clean-up
    dds::core::cond::WaitSet::ConditionSeq::iterator it;
    for (it = conditionList.begin(); it != conditionList.end(); ++it) {
        waitSet -= *it;
    }

    ddsrt_thread_join(threadId, NULL);
}

