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

#include <org/eclipse/cyclonedds/core/DeferredDestruction.hpp>

using org::eclipse::cyclonedds::core::DeferredDestruction;

std::shared_ptr<DeferredDestruction>
  DeferredDestruction::ptr;
std::mutex
  DeferredDestruction::creation_mutex;

DeferredDestruction::DeferredDestruction() : cleanup_thread(&DeferredDestruction::cleanup, this)
{

}

DeferredDestruction::~DeferredDestruction()
{
    terminate();
    cleanup_thread.join();
}

DeferredDestruction& DeferredDestruction::get_instance()
{
    if (!ptr)
    {
        std::unique_lock<std::mutex> lk(DeferredDestruction::creation_mutex);
        ptr = std::shared_ptr<DeferredDestruction>(new DeferredDestruction());
    }
    return *ptr;
}

void DeferredDestruction::cleanup()
{
    while (!terminated)
        expire();
}

void DeferredDestruction::commit()
{
    std::unique_lock<std::mutex> lk(access_mutex);
    ++n_committed;

    /* signal the cleanup thread that it may empty the list */
    if (m_entities.size() == n_committed)
        cv.notify_one();
}

std::list<dds::core::Entity>::iterator DeferredDestruction::emplace(const dds::core::Entity& e)
{
    std::unique_lock<std::mutex> lk(access_mutex);
    m_entities.emplace(m_entities.end(), e);
    return (m_entities.end())--;
}

void DeferredDestruction::erase(std::list<dds::core::Entity>::iterator it)
{
    std::unique_lock<std::mutex> lk(access_mutex);
    m_entities.erase(it);
}

void DeferredDestruction::expire()
{
    std::unique_lock<std::mutex> lk(access_mutex);

    /* wait for the signal to say it is okay to empty the list, while loop is for spurious wakes */
    while ((n_committed != m_entities.size() || n_committed == 0) && !terminated)
        cv.wait(lk);

    n_committed = 0;
    m_entities.clear();
}

void DeferredDestruction::terminate()
{
    std::unique_lock<std::mutex> lk(access_mutex);
    terminated = true;
    cv.notify_one();
}
