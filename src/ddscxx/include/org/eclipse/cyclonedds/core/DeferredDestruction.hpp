/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */


 /**
  * @file
  */

#ifndef __DEFERRED_DESTRUCTION__
#define __DEFERRED_DESTRUCTION__

#include <dds/core/Entity.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <memory>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace core
{

/**
* @brief
* This class takes care of the deferred destruction of entities.
*
* Typically it is used to allow for an entity to go out of scope
* while other entities may still have shared pointers referencing
* this entity. This causes issues when the close() function is
* invoked, giving an assert(false) failure.
*
* Typical usage:
* @code{.cpp}
* auto& dd = org::eclipse::cyclonedds::core::DeferredDestruction::get_instance();
*
* {
*     dds::core::status::RequestedDeadlineMissedStatus s;
*      s.delegate() = sd;
*
*     //this is the entity whose destruction needs to be deferred
*     dds::sub::DataReader<T, dds::sub::detail::DataReader> dr = wrapper();
*     //here it is added to deferred destruction
*     dd.emplace(dr);

*     dds::sub::DataReaderListener<T> *l =
*         reinterpret_cast<dds::sub::DataReaderListener<T> *>(this->listener_get());
*     l->on_requested_deadline_missed(dr, s);
*
*     //dr goes out of scope here, but its destruction is deferred
* }
*
* //its true destruction can only happen here
* dd.commit();
* @endcode
*/
class DeferredDestruction
{
public:

    /**
    * Destructor.
    */
    ~DeferredDestruction();

    /**
    * Commit function.
    *
    * Will cause an additional managed entity to have reached its end of life.
    * Will signal the cleanup thread if the number of end of life entities equals
    * the number of administered entities, allowing it to proceed with cleanup.
    * This function should be balanced with an entity being added through
    * emplace.
    */
    void commit();

    /**
    * Emplacement function.
    *
    * Will provisionally add an entity for deferred destruction.
    * This function should be balanced with either an invocation of commit(), causing
    * the entity to have its destruction deferred, or erase(), removing the entity
    * from the deferred destruction.
    *
    * @param e The entity to have its destruction deferred
    *
    * @return std::list<dds::core::Entity>::iterator The stored entry in the list.
    */
    std::list<dds::core::Entity>::iterator emplace(const dds::core::Entity& e);

    /**
    * Erasure function.
    *
    * Will remove the supplied entry from deferred destruction.
    * Can only be used on iterators received from emplace()
    *
    * @param it The entity to remove from the container.
    */
    void erase(std::list<dds::core::Entity>::iterator it);

    /**
    * Instance retrieval/creation function.
    *
    * Will check whether an instance already exists in the shared pointer ptr,
    * if there is not, one is created and assigned to ptr.
    *
    * @return DeferredDestruction& The stored singleton instance.
    */
    static DeferredDestruction& get_instance();

private:
    std::list<dds::core::Entity> m_entities;
    std::mutex access_mutex;
    static std::mutex creation_mutex;
    std::thread cleanup_thread;
    std::condition_variable cv;
    uint32_t n_committed = 0;
    bool terminated = false;

    static std::shared_ptr<DeferredDestruction> ptr;

    /**
    * Constructor.
    *
    * Made private due to the class using a singleton type implementation.
    */
    DeferredDestruction();

    /**
    * Background cleanup function.
    *
    * The cleanup_thread will run this function on the singleton instance.
    * Keeps calling expire() as long as terminated is not set to true.
    */
    void cleanup();

    /**
    * Cleanup waiting/worker function.
    *
    * Will halt until a notification is received on cv.
    * If the number of entities in m_entities is equal to the number committed (n_committed),
    * m_entities is emptied and n_committed is set to 0.
    */
    void expire();

    /**
    * Termination function.
    *
    * Will cause the function in cleanup_thread to complete, allowing the object
    * to be destructed.
    */
    void terminate();
};

}
}
}
}

#endif // __DEFERRED_DESTRUCTION__
