/*
 * Copyright(c) 2006 to 2020 ZettaScale Technology and others
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

#include <org/eclipse/cyclonedds/core/cond/ShadowParticipant.hpp>
#include "org/eclipse/cyclonedds/core/Mutex.hpp"

org::eclipse::cyclonedds::core::cond::ShadowParticipant*
org::eclipse::cyclonedds::core::cond::ShadowParticipant::instance_ = nullptr;

org::eclipse::cyclonedds::core::Mutex
org::eclipse::cyclonedds::core::cond::ShadowParticipant::mutex_;
